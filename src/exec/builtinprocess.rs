use std::fs::File;
use std::os::unix::io::FromRawFd;
use std::io::BufReader;

use std::process::exit;

use opts;
use posix;
use sym;

use shell::Shell;

use builtins;
use builtins::Builtin;

use posix::ReadPipe;
use posix::WritePipe;
use posix::Pgid;

use exec::Child;
use exec::ProcessInner;
use exec::Process;
use exec::Arg;

// NOTE: we assume there are no Rd args if !to_exec.rd_cap, since they *should*
// have been taken care of with push_arg.  Thus we assume there is no need to
// adapt Rd args.
fn adapt_args(to_exec: &Builtin, av: Vec<Arg>) -> Vec<Arg> {
    if to_exec.bl_cap && to_exec.pat_cap {
        return av;
    }

    let mut ret = Vec::new();
    for a in av {
        match a {
            Arg::Str(s) => ret.push(Arg::Str(s)),
            Arg::Rd(rd) => ret.push(Arg::Rd(rd)),
            Arg::Bl(bv)  => {
                if to_exec.bl_cap {
                    ret.push(Arg::Bl(bv));
                } else {
                    for l in bv {
                        ret.push(Arg::Str(l));
                    }
                }
            },
            Arg::Pat(p) => {
                if to_exec.pat_cap {
                    ret.push(Arg::Pat(p));
                } else {
                    // TODO: quotes?
                    ret.push(Arg::Str(p));
                }
            }
        }
    }

    ret
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

    pub fn from_fn(f: sym::Fn) -> Self {
        BuiltinProcess {
            to_exec: builtins::fn_builtin(f),
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
            match posix::fork(opts::is_set("__tin_inter"), pgid) {
                Err(e) => {
                    // oops. gotta bail.
                    warn!("Could not fork child: {}", e);
                    None
                },
                Ok(None) => {
                    let a = self.argv;
                    let i = self.inner;
                    let te = self.to_exec;

                    if let Err(e) = i.redirect() {
                        warn!("Could not redirect: {}", e);
                        exit(e.raw_os_error().unwrap_or(7));
                    }
                    let argv = adapt_args(&te, a);
                    let r = (*te.run)(argv, sh, None);
                    exit(r);
                },
                Ok(Some(ch_pid)) => {
                    Some(Child::new(ch_pid))
                }
            }
        } else {
            let i = self.inner;
            let a = self.argv;
            let te = self.to_exec;

            unsafe {
                let br = if i.ch_stdin.is_some() {
                    Some(BufReader::new(File::from_raw_fd(i.ch_stdin.unwrap()
                                                          .into_raw())))
                } else {
                    None
                };
                let argv = adapt_args(&te, a);
                sh.status_code = (*te.run)(argv, sh, br);
            }

            None
        }
    }

    fn has_args(&self) -> bool {
        self.to_exec.name != "__blank" || self.argv.len() > 0
    }

    fn push_arg(&mut self, new_arg: Arg) -> &Process {
        if !self.to_exec.rd_cap {
            match new_arg {
                Arg::Rd(rd) => self.inner.rds.push(rd),
                _ => self.argv.push(new_arg)
            };
        } else { self.argv.push(new_arg); }
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
