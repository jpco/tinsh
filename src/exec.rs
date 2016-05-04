use std::process::Command;
use std::process::Child;
use std::process::Stdio;
use std::os::unix::io::{FromRawFd, IntoRawFd, AsRawFd};
use std::path::PathBuf;
use std::str;

use sym::ScopeSpec;
use builtins::Builtin;
use shell::Shell;
use err::warn;
use err::info;

/* * * 
 * So here's the deal:
 * BinProcesses (obviously) always fork; if the job is fg,
 * we simply block until completion of the job in a Posix-y way.
 *
 * BuiltinProcesses, however, don't--and shouldn't--always fork.
 * They fork iff they are being piped into another process, or if
 * they are in a backgrounded job.
 */


pub trait Process {
    fn exec(&self, &mut Shell, Option<i32>, bool) -> Option<Child>;
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
    fn exec(&self, sh: &mut Shell, stdin: Option<i32>, stdout: bool)
            -> Option<Child> {
        (*self.to_exec.run)(self.argv.clone(), sh);
        None
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
    fn exec(&self, sh: &mut Shell, stdin: Option<i32>, stdout: bool) -> Option<Child> {
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
            Ok(c) => Some(c),
            Err(e) => {
                warn(&format!("Could not fork '{}': {}", self.to_exec.display(), e));
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
        if self.procs.len() == 0 { return; }

        let mut stdin: Option<i32> = None;
        let mut children = Vec::new();

        let last = self.procs.len() - 1;
        for cproc in &self.procs[..last] {
            let cchld = cproc.exec(sh, stdin, true);
            stdin = if let Some(ref child) = cchld {
                if let Some(ref stdout) = child.stdout {
                    Some(stdout.as_raw_fd())
                } else {
                    None
                }
            } else {
                None
            };
            children.push(cchld);
        }
        let lproc = self.procs.last().unwrap();
        children.push(lproc.exec(sh, stdin, false));

        if fg {
            // TODO: read any_fail from symtable
            let status = self.wait(&mut children, false);
        }
    }

    fn wait(&self, children: &mut Vec<Option<Child>>, any_fail: bool) -> i32 {
        let last = children.len() - 1;
        let mut ccode = 0;
        for child in children.drain(..last) {
            if let Some(mut child) = child {
                let xc = child.wait();
                if any_fail {
                    if let Ok(xc) = xc {
                        if let Some(code) = xc.code() {
                            if code != 0 {
                                ccode = code;
                            }
                        }
                    }
                }
            }
        }

        if let Some(mut child) = children.pop().unwrap() {
            match child.wait() {
                Ok(status) => {
                    if let Some(code) = status.code() {
                        if !any_fail || code != 0 {
                            code
                        } else {
                            ccode
                        }
                    } else {
                        info("Child ended by signal");
                        143
                    }
                },
                Err(e) => {
                    warn(&format!("Could not wait for child: {}", e));
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
