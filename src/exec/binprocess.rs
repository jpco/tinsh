use std::process::exit;
use std::io::ErrorKind;
use std::path::PathBuf;
use std::ffi::CString;
use std::ffi::OsStr;
use std::os::unix::ffi::OsStrExt;

use shell::Shell;

use posix;
use opts;

use posix::ReadPipe;
use posix::WritePipe;
use posix::Pgid;

use exec::Arg;
use exec::Process;
use exec::ProcessInner;
use exec::Child;

pub struct BinProcess {
    to_exec: PathBuf,
    argv: Vec<*const i8>,
    m_args: Vec<CString>,
    inner: ProcessInner,
}

fn os2c(s: &OsStr) -> CString {
    CString::new(s.as_bytes()).unwrap_or_else(|_e| CString::new("<string-with-nul>").unwrap())
}

fn pb2c(pb: PathBuf) -> CString {
    os2c(pb.as_os_str())
}

fn str2c(s: String) -> CString {
    CString::new(s.as_bytes()).unwrap_or_else(|_e| CString::new("<string-with-nul>").unwrap())
}


impl BinProcess {
    pub fn new(cmd: &String, b: PathBuf) -> Self {
        let cb = str2c(cmd.to_owned());
        BinProcess {
            argv: vec![cb.as_ptr(), 0 as *const _],
            to_exec: b,
            m_args: vec![cb],
            inner: ProcessInner::new(),
        }
    }
}

impl Process for BinProcess {
    fn exec(self, _sh: &mut Shell, pgid: Option<Pgid>) -> Option<Child> {
        match posix::fork(opts::is_set("__tin_inter"), pgid) {
            Err(e) => {
                // oops. gotta bail.
                warn!("Could not fork child: {}", e);
                None
            }
            Ok(None) => {
                if let Err(e) = self.inner.redirect(false) {
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
                exit(e.raw_os_error().unwrap_or(22));  // EINVAL
            }
            Ok(Some(ch_pid)) => Some(Child::new(ch_pid)),
        }
    }

    // A BinProcess always has at least its first argument -- the executable
    // to be run.
    fn has_args(&self) -> bool {
        true
    }

    fn push_arg(&mut self, new_arg: Arg) -> &Process {
        // downconvert args to Strings as we add them
        let mut v = Vec::new();
        match new_arg {
            Arg::Str(s) => {
                v.push(s);
            }
            Arg::Bl(lines) => {
                for l in lines {
                    v.push(l);
                }
            }
            Arg::Rd(rd) => {
                self.inner.rds.push(rd);
            }
        }

        // TODO: this is not a perfect way to do this
        for new_arg in v {
            let arg = str2c(new_arg);
            self.argv[self.m_args.len()] = arg.as_ptr();
            self.argv.push(0 as *const _);

            // for memory correctness
            self.m_args.push(arg);
        }

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
