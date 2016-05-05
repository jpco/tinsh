use std::process::Command;
use std::process::Child;
use std::process::Stdio;
use std::process::exit;

use std::os::unix::io::{FromRawFd, IntoRawFd, AsRawFd};
use std::path::PathBuf;
use std::str;

use std::io::Read;

extern crate libc;
use std::fs::File;

use sym::ScopeSpec;
use builtins::Builtin;
use shell::Shell;
use err::warn;
use err::info;


/// Awful struct which allows for both nice Rust-y Child structs from Process structs,
/// and hunks of int file descriptors and whatnot from libc::fork.
struct Pseudochild {
    child: Option<Child>,
    child_pid: i32,
    child_stdout: Option<i32>,
    child_stdin: Option<i32>,
    child_stderr: Option<i32>
}

impl From<Child> for Pseudochild {
    fn from(ch: Child) -> Self {
        Pseudochild {
            child: Some(ch),
            child_pid: 0,
            child_stdout: None,
            child_stdin: None,
            child_stderr: None
        }
    }
}

impl Pseudochild {
    fn new(pid: i32, stdin: Option<i32>, stdout: Option<i32>, stderr: Option<i32>)
            -> Self {
        unsafe {
            Pseudochild {
                child: None,
                child_pid: pid,
                child_stdout: if let Some(stdout) = stdout {
                    Some(stdout)
                } else { None },
                child_stdin: if let Some(stdin) = stdin {
                    Some(stdin)
                } else { None },
                child_stderr: if let Some(stderr) = stderr {
                    Some(stderr)
                } else { None }
            }
        }
    }

    fn stdout(&self) -> Option<i32> {
        if let Some(ref child) = self.child {
            if let Some(ref stdout) = child.stdout {
                unsafe {
                    return Some(stdout.as_raw_fd());
                }
            } else {
                return None;
            }
        } else {
            self.child_stdout
        }
    }

    fn wait(&mut self) -> Result<i32, ()> {
        if let Some(ref mut child) = self.child {
            match child.wait() {
                Ok(s) => {
                    if let Some(c) = s.code() {
                        Ok(c)
                    } else {
                        Ok(0)
                    }
                },
                Err(_) => { Err(()) }
            }
        } else {
            unsafe {
                let mut cstat: i32 = 0;
                if libc::waitpid(self.child_pid, &mut cstat as *mut libc::c_int, 0) < 0 {
                    Err(())
                } else {
                    Ok(cstat)
                }
            }
        }
    }

    fn wait_with_output(self) -> Result<String, ()> {
        if let Some(child) = self.child {
            match child.wait_with_output() {
                Ok(o) => {
                    match str::from_utf8(&o.stdout) {
                        Ok(st) => Ok(st.to_string()),
                        Err(_) => Err(())
                    }
                },
                Err(_) => { Err(()) }
            }
        } else {
            unsafe {
                let mut out = String::new();
                let stdout = self.child_stdout.unwrap();
                let mut f = File::from_raw_fd(stdout);
                let res = f.read_to_string(&mut out);
                if libc::waitpid(self.child_pid, 0 as *mut libc::c_int, 0) < 0 {
                    Err(())
                } else {
                    match res {
                        Ok(_) => Ok(out),
                        Err(_) => Err(())
                    }
                }
            }
        }
    }
}

pub trait Process {
    fn exec(&self, &mut Shell, Option<i32>, bool, bool) -> Option<Pseudochild>;
    fn has_args(&self) -> bool;
    fn push_arg(&mut self, String) -> &Process;
}

// builtins execute functions through __fn_exec
pub struct BuiltinProcess {
    to_exec: Builtin,
    argv: Vec<String>
}

impl BuiltinProcess {
    pub fn new(b: &Builtin) -> Self {
        BuiltinProcess {
            to_exec: b.clone(),
            argv: Vec::new()
        }
    }
}

impl Process for BuiltinProcess {
    fn exec(&self, sh: &mut Shell, stdin: Option<i32>, stdout: bool, quiet: bool) -> Option<Pseudochild> {
        if stdout {
            unsafe { // we about to get like the wild west in here
                let mut fds = [0; 2];
                if libc::pipe(fds.as_mut_ptr()) != 0 {
                    return None;
                }
                let c_pid = libc::fork();
                if c_pid < 0 {
                    return None;
                } else if c_pid == 0 {
                    libc::close(fds[0]);
                    libc::dup2(fds[1], libc::STDOUT_FILENO);
                    let res = (*self.to_exec.run)(self.argv.clone(), sh, stdin);
                    libc::close(fds[1]);
                    exit(res);
                } else {
                    libc::close(fds[1]);
                    Some(Pseudochild::new(c_pid, None, Some(fds[0]), None))
                }
            }
        } else {
            (*self.to_exec.run)(self.argv.clone(), sh, stdin);
            None
        }
    }
    
    fn has_args(&self) -> bool {
        self.to_exec.name != "__blank"
    }

    fn push_arg(&mut self, new_arg: String) -> &Process {
        self.argv.push(new_arg);
        self
    }
}

impl Default for BuiltinProcess {
    fn default() -> Self {
        BuiltinProcess {
            to_exec:   Builtin::default(),
            argv:      Vec::new(),
        }
    }
}

pub struct BinProcess {
    to_exec: PathBuf,
    pub argv: Vec<String>
}

impl BinProcess {
    pub fn new(b: &PathBuf) -> Self {
        BinProcess {
            to_exec: b.clone(),
            argv: Vec::new()
        }
    }
}

impl Process for BinProcess {
    fn exec(&self, _sh: &mut Shell, stdin: Option<i32>, stdout: bool, quiet: bool) -> Option<Pseudochild> {
        let mut cmd = Command::new(self.to_exec.clone());
        cmd.args(&self.argv);

        if let Some(stdin) = stdin {
            unsafe { cmd.stdin(Stdio::from_raw_fd(stdin)); }
        } else {
            cmd.stdin(Stdio::null());
        }

        if stdout {
            cmd.stdout(Stdio::piped());
        }

        match cmd.spawn() {
            Ok(c) => Some(Pseudochild::from(c)),
            Err(e) => {
                if !quiet {
                    warn(&format!("Could not fork '{}': {}", self.to_exec.display(), e));
                }
                None
            }
        }
    }

    fn has_args(&self) -> bool {
        true
    }

    fn push_arg(&mut self, new_arg: String) -> &Process {
        self.argv.push(new_arg);
        self
    }
}

pub struct Job {
    pub procs: Vec<Box<Process>>,
    command: String
}

impl Job {
    pub fn exec(&self, sh: &mut Shell, fg: bool) {
        let mut children = self.spawn_procs(sh, false, false);
        
        if fg {
            // TODO: read any_fail from symtable
            let status = self.wait(&mut children, false);
            sh.st.set_scope("_?", status.to_string(), ScopeSpec::Global);
        }
    }

    pub fn collect(&self, sh: &mut Shell) -> String {
        let mut children = self.spawn_procs(sh, true, true);
        self.wait_collect(&mut children).trim().replace("\n", " ")
    }

    fn spawn_procs(&self, sh: &mut Shell, pipe_out: bool, quiet: bool) -> Vec<Option<Pseudochild>> {
        if self.procs.len() == 0 { return Vec::new(); }

        let mut stdin: Option<i32> = None;
        let mut children = Vec::new();

        let last = self.procs.len() - 1;
        for cproc in &self.procs[..last] {
            let cchld = cproc.exec(sh, stdin, true, quiet);
            stdin = if let Some(ref child) = cchld {
                child.stdout()
            } else {
                None
            };
            children.push(cchld);
        }
        let lproc = self.procs.last().unwrap();
        children.push(lproc.exec(sh, stdin, pipe_out, quiet));

        children
    }

    fn wait_collect(&self, children: &mut Vec<Option<Pseudochild>>) -> String {
        let last = children.len() - 1;
        for child in children.drain(..last) {
            if let Some(mut child) = child {
                child.wait(); // don't care
            }
        }

        if let Some(mut child) = children.pop().unwrap() {
            match child.wait_with_output() {
                Ok(out) => {
                    out
                },
                Err(e)  => {
                    "".to_string()
                }
            }
        } else {
            "".to_string()
        }
    }

    fn wait(&self, children: &mut Vec<Option<Pseudochild>>, any_fail: bool) -> i32 {
        let last = children.len() - 1;
        let mut ccode = 0;
        for child in children.drain(..last) {
            if let Some(mut child) = child {
                let xc = child.wait();
                if any_fail {
                    if let Ok(xc) = xc {
                        if xc != 0 {
                            ccode = xc;
                        }
                    }
                }
            }
        }

        if let Some(mut child) = children.pop().unwrap() {
            match child.wait() {
                Ok(status) => {
                    if !any_fail || status != 0 {
                        status
                    } else {
                        ccode
                    }
                },
                Err(_) => {
                    warn(&format!("Could not wait for child."));
                    127
                }
            }
        } else {
            2
        }
    }

    pub fn new(cmd: String) -> Self {
        Job {
            procs: Vec::new(),
            command: cmd
        }
    }
}
