// TODO: make binprocess and builtinprocess *NOT* public, create a helper function
// in this module to decide which proc to use
pub mod binprocess;
pub mod builtinprocess;
pub mod job;

use std::mem;
use std::fs::OpenOptions;
use std::any::Any;
use std::io::Result;
use std::os::unix::io::AsRawFd;

use posix;

use posix::ReadPipe;
use posix::WritePipe;
use posix::Pgid;
use posix::Pid;

use shell::Shell;

use self::ProcStruct::BuiltinProc;
use self::ProcStruct::BinProc;
use exec::binprocess::BinProcess;
use exec::builtinprocess::BuiltinProcess;

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
    Pat(String),
    Rd(Redir)
}

impl Arg {
    pub fn is_str(&self) -> bool {
        match *self {
            Arg::Str(_) => true,
            _      => false
        }
    }

    pub fn as_str(&self) -> &str {
        match *self {
            Arg::Str(ref s) => s,
            _      => panic!()
        }
    }

    pub fn unwrap_str(self) -> String {
        match self {
            Arg::Str(s) => s,
            _      => panic!()
        }
    }


    pub fn is_bl(&self) -> bool {
        match *self {
            Arg::Bl(_)  => true,
            _      => false
        }
    }


    pub fn unwrap_bl(self) -> Vec<String> {
        match self {
            Arg::Bl(bv)  => bv,
            _      => panic!()
        }
    }


    pub fn is_pat(&self) -> bool {
        match *self {
            Arg::Pat(_) => true,
            _      => false
        }
    }

    pub fn unwrap_pat(self) -> String {
        match self {
            Arg::Pat(p) => p,
            _      => panic!()
        }
    }


    pub fn is_rd(&self) -> bool {
        match *self {
            Arg::Rd(_)  => true,
            _      => false
        }
    }

    pub fn unwrap_rd(self) -> Redir {
        match self {
            Arg::Rd(rd)  => rd,
            _      => panic!()
        }
    }

    pub fn into_vec(self) -> Vec<String> {
        let mut ret = Vec::new();

        match self {
            Arg::Str(s) => ret.push(s),
            Arg::Bl(bl) => for l in bl { ret.push(l) },
            Arg::Pat(p) => ret.push(p), // FIXME?
            Arg::Rd(rd) => {
                match rd {
                    // FIXME: should these be single quotes here?
                    Redir::RdArgOut(s) => {
                        ret.push("~>".to_string());
                        ret.push(s);
                    },
                    Redir::RdArgIn(s) => {
                        ret.push("<~".to_string());
                        ret.push(s);
                    },
                    Redir::RdFdOut(a, b) => {
                        ret.push(if a == -2 {
                            format!("-&>{}", b)
                        } else {
                            format!("-{}>{}", a, b)
                        });
                    },
                    Redir::RdFdIn(a, b) => ret.push(format!("{}<{}-", a, b)),
                    Redir::RdFileOut(a, dest, ap) => {
                        ret.push(format!("-{}>{}", a, if ap { "+" } else { "" }));
                        ret.push(dest);
                    },
                    Redir::RdFileIn(a, src) => {
                        ret.push(format!("{}<-", a));
                        ret.push(src);
                    },
                    Redir::RdStringIn(a, src) => {
                        ret.push(format!("{}<<-", a));
                        ret.push(src);
                    }
                }
            }
        }

        ret
    }
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
