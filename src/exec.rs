use std::process::Command;
use std::process::Stdio;
use std::path::PathBuf;

use sym;
use builtins::Builtin;
use shell::Shell;
use err::warn;

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
    fn fork_exec(&self, Option<i32>, bool) -> Option<i32>;
    fn local_exec(&self, Option<i32>, &mut Shell);
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
    fn fork_exec(&self, stdin: Option<i32>, pipe_out: bool) -> Option<i32> {
        println!("Not sure how to do that yet, tbh.");
        None
    }
    
    fn local_exec(&self, stdin: Option<i32>, sh: &mut Shell) {
        let c = (*self.to_exec.run)(self.argv.clone(), sh);
        sh.st.set_scope("_?", c.to_string(), sym::ScopeSpec::Global);
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
    fn fork_exec(&self, stdin: Option<i32>, pipe_out: bool) -> Option<i32> {
        let mut cmd = Command::new(self.to_exec.clone());
        cmd.args(&self.argv);

        if let Err(e) = cmd.spawn() {
            warn(&format!("Could not fork '{}': {}", self.to_exec.display(), e));
        }

        None
    }

    fn local_exec(&self, stdin: Option<i32>, sh: &mut Shell) {
        let mut cmd = Command::new(self.to_exec.clone());
        cmd.args(&self.argv);

        match cmd.status() {
            Ok(s) => {
                if let Some(c) = s.code() {
                    sh.st.set_scope("_?", c.to_string(), sym::ScopeSpec::Global);
                }
            },
            Err(e) => {
                warn(&format!("Could not fork '{}': {}", self.to_exec.display(), e));
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

        let last = self.procs.len() - 1;
        for cproc in &self.procs[..last] {
            cproc.fork_exec(None, true);
        }
        let lproc = self.procs.last().unwrap();
        if fg {
            lproc.local_exec(None, sh);
        } else {
            lproc.fork_exec(None, false);
        }
    }

    pub fn new(cmd: String) -> Self {
        Job {
            procs: Vec::new(),
            command: cmd
        }
    }
}
