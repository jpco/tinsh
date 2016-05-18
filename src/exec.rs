use std::path::PathBuf;
use std::process::exit;
use std::any::Any;

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
use err::warn;
use builtins::Builtin;

use self::ProcStruct::BuiltinProc;
use self::ProcStruct::BinProc;

pub enum ProcStruct {
    BuiltinProc(BuiltinProcess),
    BinProc(BinProcess),
}

/// Struct describing a child process.  Allows the shell to wait for the process and
/// control the process' inputs and outputs.
pub struct Child {
    pid: Pid,
    stdout: Option<ReadPipe>,
    stdin:  Option<WritePipe>
}

impl Child {
    fn new(pid: Pid) -> Self {
        Child {
            pid: pid,
            stdout: None,
            stdin: None
        }
    }
}

pub trait Process : Any {
    fn exec(self, &mut Shell, Option<Pgid>, bool) -> Option<Child>;
    fn has_args(&self) -> bool;
    fn push_arg(&mut self, String) -> &Process;
    fn stdin(&mut self, ReadPipe) -> &Process;
    fn stdout(&mut self, WritePipe) -> &Process;
}

impl Process for Box<ProcStruct> {
    fn exec(self, sh: &mut Shell, pgid: Option<Pgid>, tt: bool) -> Option<Child> {
        match *self {
            BuiltinProc(bp) => { bp.exec(sh, pgid, tt) },
            BinProc(bp)     => { bp.exec(sh, pgid, tt) }
        }
    }
    fn has_args(&self) -> bool {
        match **self {
            BuiltinProc(ref bp) => { bp.has_args() },
            BinProc(ref bp)     => { true }
        }
    }
    fn push_arg(&mut self, arg: String) -> &Process {
        match **self {
            BuiltinProc(ref mut bp) => { bp.push_arg(arg) },
            BinProc(ref mut bp)     => { bp.push_arg(arg) }
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

/// Struct containing 
struct ProcessInner {
    ch_stdin:  Option<ReadPipe>,
    ch_stdout: Option<WritePipe>
}

impl ProcessInner {
    fn new() -> Self {
        ProcessInner {
            ch_stdin:  None,
            ch_stdout: None
        }
    }

    fn redirect(self) {
        if let Some(read) = self.ch_stdin {
            posix::set_stdin(read).expect("Could not redirect stdin");
        }
        if let Some(write) = self.ch_stdout {
            posix::set_stdout(write).expect("Could not redirect stdout");
        }
    }
}

/// Struct representing a builtin to be executed -- essentially, anything which does
/// not require a call to execv to run.
pub struct BuiltinProcess {
    to_exec: Builtin,
    argv: Vec<String>,
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
    fn exec(self, sh: &mut Shell, _pgid: Option<Pgid>, _tt: bool) -> Option<Child> {
        // TODO: these need to fork sometimes
        (*self.to_exec.run)(self.argv.to_owned(), sh, None);
        None
    }

    fn has_args(&self) -> bool {
        self.to_exec.name != "__blank"
    }

    fn push_arg(&mut self, new_arg: String) -> &Process {
        self.argv.push(new_arg);
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


fn str2c(s: String) -> CString {
    CString::new(s.as_bytes()).unwrap_or_else(|_e| {
        CString::new("<string-with-nul>").unwrap()
    })
}


impl BinProcess {
    pub fn new(b: PathBuf) -> Self {
        let cb = os2c(b.as_os_str());
        BinProcess {
            argv: vec![cb.as_ptr(), 0 as *const _],
            to_exec: b,
            m_args: vec![cb],
            inner: ProcessInner::new()
        }
    }
}

impl Process for BinProcess {
    fn exec(self, sh: &mut Shell, pgid: Option<Pgid>, tt: bool) -> Option<Child> {
        // TODO: set up I/O
        match posix::fork(sh.interactive, pgid) {
            Err(e) => {
                // oops. gotta bail.
                warn(&format!("Could not fork child: {}", e));
                None
            },
            Ok(None) => {
                // gotta exec!
                // TODO: perform redirections in child
                self.inner.redirect();
                let e = posix::execv(self.argv[0], self.argv.as_ptr());
                warn(&format!("Could not exec: {}", e));
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

    fn push_arg(&mut self, new_arg: String) -> &Process {
        let arg = str2c(new_arg);
        self.argv[self.m_args.len()] = arg.as_ptr();
        self.argv.push(0 as *const _);

        // for memory correctness
        self.m_args.push(arg);

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

    command: String,
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
                        warn(&format!("Could not create pipe: {}", e));
                    }
                }
            }
            let tt = i == pr_len && self.fg;
            if let Some(ch) = cproc.exec(sh, self.pgid, tt) {
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
                warn(&format!("Could not give child the terminal: {}", e));
            }
        }

        self.spawned = true;
    }

    pub fn wait_collect(&mut self) -> String {
        assert!(self.do_pipe_out && self.spawned);
        let ch_len = self.children.len();

        for ch in self.children.drain(..ch_len) {
            let _ = posix::wait_pid(&ch.pid, None);
        }

        // **should** always succeed
        let r = if let Some(ch) = self.children.pop() {
            let mut buf = String::new();
            match posix::wait_pid(&ch.pid, Some(&mut buf)) {
                Ok(_) => {
                    buf
                },
                Err(e) => {
                    warn(&format!("Error waiting for child: {}", e));
                    "".to_string()
                }
            }
        } else { "".to_string() };

        let _ = posix::set_signal_ignore(true);
        r
    }

    pub fn wait(&mut self) -> Option<Status> {
        assert!(self.spawned && !self.do_pipe_out);
        let ch_len = self.children.len();
        for ch in self.children.drain(..ch_len) {
            let _ = posix::wait_pid(&ch.pid, None);
        }

        // **should** always succeed
        let r = if let Some(ch) = self.children.pop() {
            match posix::wait_pid(&ch.pid, None) {
                Ok(st) => {
                    st
                },
                Err(e) => {
                    warn(&format!("Error waiting for child: {}", e));
                    None
                }
            }
        } else { None };

        // do this before taking the terminal to prevent indefinite hang
        if let Err(e) = posix::set_signal_ignore(true) {
            warn(&format!("Couldn't ignore signals: {}", e));
        } else {
            if let Err(e) = posix::take_terminal() {
                warn(&format!("Couldn't take terminal: {}", e));
            }
        }
        r
    }

    pub fn new(cmd: String) -> Self {
        Job {
            spawned: false,
            procs: Vec::new(),
            children: Vec::new(),

            command: cmd,
            pgid: None,
            fg: true,

            do_pipe_out: false,
            pipe_out: None
        }
    }
}
