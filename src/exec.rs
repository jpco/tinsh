use std::path::PathBuf;
use std::process::exit;
use std::any::Any;
use std::io::Read;
use std::io::ErrorKind;
use std::io::Result;
use std::io::BufReader;
use std::fs::OpenOptions;
use std::fs::File;
use std::os::unix::io::AsRawFd;
use std::os::unix::io::FromRawFd;
use std::mem;

use std::ffi::CString;
use std::ffi::OsStr;
use std::os::unix::ffi::OsStrExt;

use posix;
use posix::Pid;
use posix::Pgid;
use posix::ReadPipe;
use posix::WritePipe;
use posix::Status;

// oh well. keep this minimal at least.
extern crate libc;
use self::libc::c_char;
use self::libc::EINVAL;

use shell::Shell;
use builtins::Builtin;

use self::ProcStruct::BuiltinProc;
use self::ProcStruct::BinProc;

// out grammar: (-|~)(fd|&)?>(fd|+)?
// in grammar:  (fd)?<<?(fd)?(-|~)
pub enum Redir {
    RdArgOut(String),             // command substitutions
    RdArgIn(String),              //  - out/in controls WritePipe vs ReadPipe
    RdFdOut(i32, i32),  // fd substitutions
    RdFdIn(i32, i32),   //  - e.g. -2>1
    RdFileOut(i32, String, bool),  // file substitutions
    RdFileIn(i32, String),   //  - e.g. -2> errs.txt
    RdStringIn(i32, String)  // here-string/here-documents
}

pub enum Arg {
    Str(String),
    Bl(Vec<String>),
    Pat(String)
}

pub fn downconvert(args: Vec<Arg>) -> Vec<String> {
    let mut v = Vec::new();
    for a in args {
        match a {
            Arg::Str(s) => {
                v.push(s);
            },
            Arg::Bl(lines) => {
                for l in lines {
                    if !l.trim().is_empty() {
                        v.push(l.trim().to_owned());
                    }
                }
            },
            Arg::Pat(p) => {
                // TODO: properly process p
                v.push(p);
            }
        }
    }

    v
}

/// Struct describing a child process.  Allows the shell to wait for the process and
/// control the process' input & output.
pub struct Child {
    pid: Pid
}

impl Child {
    fn new(pid: Pid) -> Self {
        Child {
            pid: pid
        }
    }
}

pub trait Process : Any {
    fn exec(self, &mut Shell, Option<Pgid>) -> Option<Child>;
    fn has_args(&self) -> bool;
    fn push_arg(&mut self, Arg) -> &Process;
    fn push_redir(&mut self, Redir) -> &Process;
    fn stdin(&mut self, ReadPipe) -> &Process;
    fn stdout(&mut self, WritePipe) -> &Process;
}

pub enum ProcStruct {
    BuiltinProc(BuiltinProcess),
    BinProc(BinProcess),
}

impl Process for Box<ProcStruct> {
    fn exec(self, sh: &mut Shell, pgid: Option<Pgid>) -> Option<Child> {
        match *self {
            BuiltinProc(bp) => { bp.exec(sh, pgid) },
            BinProc(bp)     => { bp.exec(sh, pgid) }
        }
    }
    fn has_args(&self) -> bool {
        match **self {
            BuiltinProc(ref bp) => { bp.has_args() },
            BinProc(_)     => { true }
        }
    }
    fn push_arg(&mut self, arg: Arg) -> &Process {
        match **self {
            BuiltinProc(ref mut bp) => { bp.push_arg(arg) },
            BinProc(ref mut bp)     => { bp.push_arg(arg) }
        };
        self
    }
    fn push_redir(&mut self, redir: Redir) -> &Process {
        match **self {
            BuiltinProc(ref mut bp) => { bp.push_redir(redir) },
            BinProc(ref mut bp)     => { bp.push_redir(redir) }
        };
        self
    }
    fn stdin(&mut self, arg: ReadPipe) -> &Process {
        match **self {
            BuiltinProc(ref mut bp) => { bp.stdin(arg) },
            BinProc(ref mut bp)     => { bp.stdin(arg) }
        };
        self
    }
    fn stdout(&mut self, arg: WritePipe) -> &Process {
        match **self {
            BuiltinProc(ref mut bp) => { bp.stdout(arg) },
            BinProc(ref mut bp)     => { bp.stdout(arg) }
        };
        self
    }
}

struct ProcessInner {
    ch_stdin:  Option<ReadPipe>,
    ch_stdout: Option<WritePipe>,
    rds:       Vec<Redir>
}

impl ProcessInner {
    fn new() -> Self {
        ProcessInner {
            ch_stdin:  None,
            ch_stdout: None,
            rds:       Vec::new()
        }
    }

    fn redirect(mut self) -> Result<()> {
        if let Some(read) = self.ch_stdin {
            try!(posix::set_stdin(read));
        }
        if let Some(write) = self.ch_stdout {
            try!(posix::set_stdout(write));
        }

        while let Some(rd) = self.rds.pop() {
            match rd {
                Redir::RdArgOut(dest) => {
                    // unimplemented
                    println!(" ~> '{}'", dest);
                },
                Redir::RdArgIn(src) => {
                    // unimplemented
                    println!(" <~ '{}'", src);
                },
                Redir::RdFdOut(src, dest) => {
                    try!(posix::dup_fds(src, dest));
                    if src == -2 { // '&'
                        try!(posix::dup_fds(1, dest));
                        try!(posix::dup_fds(2, dest));
                    } else {
                        try!(posix::dup_fds(src, dest));
                    }
                },
                Redir::RdFdIn(src, dest) => {
                    try!(posix::dup_fds(src, dest));
                },
                Redir::RdFileOut(src, dest, app) => {
                    let fi = try!(OpenOptions::new().write(true).create(true)
                                                    .append(app).open(dest));
                    if src == -2 { // '&'
                        try!(posix::dup_fds(1, fi.as_raw_fd()));
                        try!(posix::dup_fds(2, fi.as_raw_fd()));
                    } else {
                        try!(posix::dup_fds(src, fi.as_raw_fd()));
                    }
                    mem::forget(fi);
                },
                Redir::RdFileIn(dest, src) => {
                    let fi = try!(OpenOptions::new().read(true).open(src));
                    try!(posix::dup_fds(dest, fi.as_raw_fd()));
                    mem::forget(fi);
                },
                Redir::RdStringIn(dest, src_str) => {
                    // unimplemented
                    println!(" string '{}' => fd '{}'", src_str, dest);
                }
            }
        }
        Ok(())
    }
}

/// Struct representing a builtin to be executed -- essentially, anything which does
/// not require a call to execv to run.
pub struct BuiltinProcess {
    to_exec: Builtin,
    argv: Vec<Arg>,
    inner: ProcessInner
}

impl BuiltinProcess {
    pub fn new(b: Builtin) -> Self {
        BuiltinProcess {
            to_exec: b,
            argv: Vec::new(),
            inner: ProcessInner::new()
        }
    }
}

impl Process for BuiltinProcess {
    fn exec(self, sh: &mut Shell, pgid: Option<Pgid>) -> Option<Child> {
        // TODO: fork happens iff
        //  - proc is bg
        //  - stdout is some
        //  - there are output redirects
        if self.inner.ch_stdout.is_some() {
            match posix::fork(sh.interactive, pgid) {
                Err(e) => {
                    // oops. gotta bail.
                    warn!("Could not fork child: {}", e);
                    None
                },
                Ok(None) => {
                    // TODO: perform redirections in child
                    if let Err(e) = self.inner.redirect() {
                        warn!("Could not redirect: {}", e);
                        exit(e.raw_os_error().unwrap_or(7));
                    }
                    let r = (*self.to_exec.run)(self.argv, sh, None);
                    exit(r);
                },
                Ok(Some(ch_pid)) => {
                    Some(Child::new(ch_pid))
                }
            }
        } else {
            unsafe {
                let br = if self.inner.ch_stdin.is_some() {
                    Some(BufReader::new(File::from_raw_fd(self
                                                          .inner
                                                          .ch_stdin
                                                          .unwrap()
                                                          .into_raw())))
                } else {
                    None
                };
                (*self.to_exec.run)(self.argv, sh, br);
            }

            None
        }
    }

    fn has_args(&self) -> bool {
        self.to_exec.name != "__blank" || self.argv.len() > 0
    }

    fn push_arg(&mut self, new_arg: Arg) -> &Process {
        self.argv.push(new_arg);
        self
    }

    fn push_redir(&mut self, new_redir: Redir) -> &Process {
        self.inner.rds.push(new_redir);
        self
    }

    fn stdin(&mut self, read: ReadPipe) -> &Process {
        self.inner.ch_stdin = Some(read);
        self
    }
    
    fn stdout(&mut self, write: WritePipe) -> &Process {
        self.inner.ch_stdout = Some(write);
        self
    }
}

impl Default for BuiltinProcess {
    fn default() -> Self {
        BuiltinProcess {
            to_exec:   Builtin::default(),
            argv:      Vec::new(),
            inner:     ProcessInner::new()
        }
    }
}

pub struct BinProcess {
    to_exec: PathBuf,
    argv: Vec<*const c_char>,
    m_args: Vec<CString>,
    inner: ProcessInner
}

fn os2c(s: &OsStr) -> CString {
    CString::new(s.as_bytes()).unwrap_or_else(|_e| {
        CString::new("<string-with-nul>").unwrap()
    })
}

fn pb2c(pb: PathBuf) -> CString {
    os2c(pb.as_os_str())
}

fn str2c(s: String) -> CString {
    CString::new(s.as_bytes()).unwrap_or_else(|_e| {
        CString::new("<string-with-nul>").unwrap()
    })
}


impl BinProcess {
    pub fn new(cmd: &String, b: PathBuf) -> Self {
        let cb = str2c(cmd.to_owned());
        BinProcess {
            argv: vec![cb.as_ptr(), 0 as *const _],
            to_exec: b,
            m_args: vec![cb],
            inner: ProcessInner::new()
        }
    }
}

impl Process for BinProcess {
    fn exec(self, sh: &mut Shell, pgid: Option<Pgid>) -> Option<Child> {
        match posix::fork(sh.interactive, pgid) {
            Err(e) => {
                // oops. gotta bail.
                warn!("Could not fork child: {}", e);
                None
            },
            Ok(None) => {
                if let Err(e) = self.inner.redirect() {
                    warn!("Could not redirect: {}", e);
                    exit(e.raw_os_error().unwrap_or(7));
                }

                let e = posix::execv(pb2c(self.to_exec).as_ptr(), self.argv.as_ptr());
                if e.kind() == ErrorKind::NotFound {
                    // TODO: custom handler function
                    warn!("Command '{}' not found.", self.m_args[0].to_str().unwrap());
                } else {
                    warn!("Could not exec: {}", e);
                }
                exit(e.raw_os_error().unwrap_or(EINVAL));
            },
            Ok(Some(ch_pid)) => {
                Some(Child::new(ch_pid))
            }
        }
    }

    // A BinProcess always has at least its first argument -- the executable
    // to be run.
    fn has_args(&self) -> bool { true }

    fn push_arg(&mut self, new_arg: Arg) -> &Process {
        // downconvert args to Strings as we add them
        let mut v = Vec::new();
        match new_arg {
            Arg::Str(s) => {
                v.push(s);
            },
            Arg::Bl(lines) => {
                for l in lines {
                    v.push(l);
                }
            },
            Arg::Pat(p) => {
                // TODO: properly process p
                v.push(p);
            }
        }

        for new_arg in v {
            let arg = str2c(new_arg);
            self.argv[self.m_args.len()] = arg.as_ptr();
            self.argv.push(0 as *const _);

            // for memory correctness
            self.m_args.push(arg);
        }

        self
    }

    fn push_redir(&mut self, new_redir: Redir) -> &Process {
        self.inner.rds.push(new_redir);
        self
    }

    fn stdin(&mut self, read: ReadPipe) -> &Process {
        self.inner.ch_stdin = Some(read);
        self
    }
    
    fn stdout(&mut self, write: WritePipe) -> &Process {
        self.inner.ch_stdout = Some(write);
        self
    }
}

pub struct Job {
    spawned: bool,
    // TODO: make procs private
    pub procs: Vec<Box<ProcStruct>>,
    children: Vec<Child>,

    // TODO: to be used with job control
    // command: String,
    pgid: Option<Pgid>,
    pub fg: bool,

    pub do_pipe_out: bool,
    pipe_out: Option<ReadPipe>
}

impl Job {
    pub fn spawn(&mut self, sh: &mut Shell) {
        assert!(!self.spawned);
        let pr_len = self.procs.len();
        let mut i = 0;

        let mut read = None;
        for mut cproc in self.procs.drain(..) {
            i = i + 1;
            if let Some(read) = read {
                cproc.stdin(read);
            }
            read = None;

            if i < pr_len || self.do_pipe_out {
                match posix::pipe() {
                    Ok((nread, write)) => {
                        cproc.stdout(write);
                        read = Some(nread);
                    },
                    Err(e) => {
                        warn!("Could not create pipe: {}", e);
                    }
                }
            }
            if let Some(ch) = cproc.exec(sh, self.pgid) {
                if sh.interactive {
                    if self.pgid.is_none() {
                        self.pgid = Some(ch.pid.to_pgid());
                    }
                }
                self.children.push(ch);
            }
        }
        if self.do_pipe_out {
            self.pipe_out = read;
        }

        if self.fg && sh.interactive && self.pgid.is_some() {
            if let Err(e) = posix::give_terminal(self.pgid.unwrap()) {
                warn!("Could not give child the terminal: {}", e);
            }
        }

        self.spawned = true;
    }

    pub fn wait_collect(&mut self, sh: &Shell) -> String {
        assert!(self.spawned && self.do_pipe_out);
        let ch_len = match self.children.len() {
            0 => { return "".to_string(); },
            x => x - 1
        };

        for ch in self.children.drain(..ch_len) {
            let _ = posix::wait_pid(&ch.pid);
        }

        // this if let **should** always succeed
        let r = if let Some(ch) = self.children.pop() {
            let mut buf = String::new();
            let mut read = mem::replace(&mut self.pipe_out, None).unwrap();
            match read.read_to_string(&mut buf) {
                Ok(_len) => {
                    while buf.ends_with('\n') {
                        buf.pop();
                    }
                    buf = buf.replace('\n', " ");
                },
                Err(e) => {
                    println!("Error reading from child: {}", e);
                }
            }
            if let Err(e) = posix::wait_pid(&ch.pid) {
                println!("Error waiting for child: {}", e);
            }
            buf
        } else { "".to_string() };

        if sh.interactive {
            if let Err(e) = posix::set_signal_ignore(true) {
                warn!("Couldn't ignore signals: {}", e);
            } else {
                if let Err(e) = posix::take_terminal() {
                    warn!("Couldn't take terminal: {}", e);
                }
            }
        }

        r
    }

    pub fn wait(&mut self, sh: &Shell) -> Option<Status> {
        assert!(self.spawned && !self.do_pipe_out);
        let ch_len = match self.children.len() {
            0 => { return None; },
            x => x - 1
        };

        for ch in self.children.drain(..ch_len) {
            let _ = posix::wait_pid(&ch.pid);
        }

        // **should** always succeed
        let r = if let Some(ch) = self.children.pop() {
            match posix::wait_pid(&ch.pid) {
                Ok(st) => {
                    st
                },
                Err(e) => {
                    warn!("Error waiting for child: {}", e);
                    None
                }
            }
        } else { None };

        if sh.interactive {
            // do this before taking the terminal to prevent indefinite hang
            if let Err(e) = posix::set_signal_ignore(true) {
                warn!("Couldn't ignore signals: {}", e);
            } else {
                if let Err(e) = posix::take_terminal() {
                    warn!("Couldn't take terminal: {}", e);
                }
            }
        }

        r
    }

    pub fn new(_cmd: String) -> Self {
        Job {
            spawned: false,
            procs: Vec::new(),
            children: Vec::new(),

            // command: cmd,
            pgid: None,
            fg: true,

            do_pipe_out: false,
            pipe_out: None
        }
    }
}
