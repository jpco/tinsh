#![allow(dead_code)]
// New rule: only posix.rs gets to use libc.

extern crate libc;

use std::process::exit;

use std::mem;

use std::ops::Drop;

use std::io;
use std::io::Result;
use std::io::Read;
use std::io::Write;
use std::io::Error;
use std::io::ErrorKind;

use std::fmt;

use std::os::unix::io::AsRawFd;

use std::fs::File;

extern "C" {
    fn tcsetpgrp(fd: libc::c_int, prgp: libc::pid_t) -> libc::c_int;
}

// eek!
#[repr(C)]
struct Repr<T> {
    data: *const T,
    len: usize,
}

macro_rules! etry {
    ($e:expr) => {{
        let res = $e;
        if res < 0 {
            return Err(Error::last_os_error());
        }
        res
    }}
}

// Note that we can't derive Clone/Copy on FileDesc: since we use drop
// to close the file, making copies will give us unexpected fd closure.

/// Struct representing a file descriptor.
pub struct FileDesc(i32);

/// Struct representing the readable side of a pipe.
pub struct ReadPipe(FileDesc);

/// Struct representing the writable side of a pipe.
pub struct WritePipe(FileDesc);

/// Struct representing a process' pgid
#[derive(Clone, Copy)]
pub struct Pgid(i32);

/// Struct representing a process' pid
#[derive(Clone, Copy, Debug)]
pub struct Pid(i32);

/// Struct representing a process' status
#[derive(Clone, Copy)]
pub struct Status(i32);

impl Status {
    pub fn to_int(&self) -> i32 {
        self.0
    }
    pub fn to_string(&self) -> String {
        self.0.to_string()
    }
}

impl Pid {
    pub fn current() -> i32 {
        unsafe { libc::getpid() }
    }

    pub fn to_pgid(&self) -> Pgid {
        Pgid(self.0)
    }
}

impl fmt::Display for FileDesc {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl Read for FileDesc {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        let ret = etry!(unsafe {
            libc::read(self.0,
                       buf.as_mut_ptr() as *mut libc::c_void,
                       buf.len() as libc::size_t)
        });
        Ok(ret as usize)
    }

    fn read_to_end(&mut self, buf: &mut Vec<u8>) -> io::Result<usize> {
        // this is essentially a copy of read_to_end_uninitialized from std::io
        let start_len = buf.len();
        buf.reserve(16);
        loop {
            if buf.len() == buf.capacity() {
                buf.reserve(1);
            }

            // from_raw_parts_mut from libcore
            unsafe {
                let buf_slice = mem::transmute(Repr {
                    data: buf.as_mut_ptr().offset(buf.len() as isize),
                    len: buf.capacity() - buf.len(),
                });

                match self.read(buf_slice) {
                    Ok(0) => {
                        return Ok(buf.len() - start_len);
                    }
                    Ok(n) => {
                        let len = buf.len() + n;
                        buf.set_len(len);
                    }
                    Err(ref e) if e.kind() == ErrorKind::Interrupted => {}
                    Err(e) => {
                        return Err(e);
                    }
                }
            }
        }
    }
}

impl Write for FileDesc {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        let ret = etry!(unsafe {
            libc::write(self.0,
                        buf.as_ptr() as *const libc::c_void,
                        buf.len() as libc::size_t)
        });
        Ok(ret as usize)
    }

    fn flush(&mut self) -> Result<()> {
        // given we are only using FileDesc structs for pipes,
        // flush isn't important (TODO: confirm this?)
        Ok(())
    }
}

impl FileDesc {
    pub fn new(n: i32) -> Self {
        FileDesc(n)
    }

    pub fn into_raw(self) -> i32 {
        let fd = self.0;
        mem::forget(self);
        fd
    }
}

impl Drop for FileDesc {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.0) };
    }
}

impl From<File> for FileDesc {
    fn from(f: File) -> Self {
        let fd = f.as_raw_fd();
        mem::forget(f);
        FileDesc(fd)
    }
}

impl ReadPipe {
    pub fn into_raw(self) -> i32 {
        self.0.into_raw()
    }

    pub fn close(self) {
        drop(self)
    }
}

impl WritePipe {
    pub fn into_raw(self) -> i32 {
        self.0.into_raw()
    }

    pub fn close(self) {
        drop(self)
    }
}

impl Read for ReadPipe {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        self.0.read(buf)
    }
}

impl Write for WritePipe {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        self.0.write(buf)
    }

    fn flush(&mut self) -> Result<()> {
        self.0.flush()
    }
}

/// Initialize the shell in a POSIX-pleasing way
pub fn init() {
    // wait until we are in the fg
    unsafe {
        let pgid = libc::getpgrp();
        while libc::tcgetpgrp(libc::STDIN_FILENO) != pgid {
            libc::kill(-libc::STDIN_FILENO, libc::SIGTTIN);
        }
    }

    // ignore interactive signals && job control signals
    if let Err(e) = set_signal_ignore(true) {
        println!("Could not set signals as ignored: {}", e);
    }

    // guarantee we are in charge of the terminal/we are process group leader
    unsafe {
        let pgid = libc::getpid();
        if pgid != libc::getpgrp() {
            if libc::setpgid(pgid, pgid) < 0 {
                err!("Couldn't put shell in its own process group: {}",
                     Error::last_os_error());
            }
        }
    }

    if let Err(e) = take_terminal() {
        err!("Couldn't take the terminal: {}", e);
    }
}

/// Returns true iff stdin is a TTY; or, equivalently, if the
/// shell is to be interactive.
pub fn is_interactive() -> bool {
    unsafe {
        // since stdin is always a valid fd, will only return 0
        // if stdin is in fact not a TTY
        libc::isatty(libc::STDIN_FILENO) != 0
    }
}

/// (Attempts to) create a pipe.
pub fn pipe() -> io::Result<(ReadPipe, WritePipe)> {
    unsafe {
        let mut fds = [0; 2];
        etry!(libc::pipe(fds.as_mut_ptr()));
        Ok((ReadPipe(FileDesc(fds[0])), WritePipe(FileDesc(fds[1]))))
    }
}

/// Waits for any child process to finish.
pub fn wait_any() -> Result<(Pid, Option<Status>)> {
    unimplemented!();
}

/// Waits for any child from the given process group to finish.
pub fn wait_pgid(_group: &Pgid) -> Result<(Pid, Option<Status>)> {
    unimplemented!();
}


/// Waits for a child process to finish.
pub fn wait_pid(child: &Pid) -> Result<Option<Status>> {
    unsafe {
        let mut st: i32 = 0;
        if libc::waitpid(child.0, &mut st, 0) < 0 {
            if Error::last_os_error().kind() == ErrorKind::Interrupted {
                Ok(None)
            } else {
                Err(Error::last_os_error())
            }
        } else {
            let st = st >> 8;
            Ok(Some(Status(st)))
        }
    }
}

/// Set stdin to be the current pipe.
pub fn set_stdin(pipe: ReadPipe) -> Result<()> {
    unsafe {
        let fd = pipe.into_raw();
        etry!(libc::dup2(fd, libc::STDIN_FILENO));
    };
    Ok(())
}

/// Set stdout to be the current pipe.
pub fn set_stdout(pipe: WritePipe) -> Result<()> {
    unsafe {
        let fd = pipe.into_raw();
        etry!(libc::dup2(fd, libc::STDOUT_FILENO));
    };
    Ok(())
}

pub fn dup_fd(src: i32) -> Result<i32> {
    let res = unsafe { etry!(libc::dup(src)) };
    Ok(res)
}

pub fn dup_stdin() -> Result<ReadPipe> {
    let n_fd = try!(dup_fd(0));
    Ok(ReadPipe(FileDesc(n_fd)))
}

pub fn dup_stdout() -> Result<WritePipe> {
    let n_fd = try!(dup_fd(1));
    Ok(WritePipe(FileDesc(n_fd)))
}

pub fn dup_fds(src: i32, dest: i32) -> Result<()> {
    unsafe {
        etry!(libc::dup2(dest, src));
    }
    Ok(())
}

pub fn set_signal_ignore(ignore: bool) -> Result<()> {
    macro_rules! stry {
        ($e:expr) => {{
            let res = $e;
            if res == libc::SIG_ERR {
                return Err(Error::last_os_error());
            }
            res
        }}
    }

    let action = if ignore {
        libc::SIG_IGN
    } else {
        libc::SIG_DFL
    };
    unsafe {
        stry!(libc::signal(libc::SIGINT, action));
        stry!(libc::signal(libc::SIGQUIT, action));
        stry!(libc::signal(libc::SIGTSTP, action));
        stry!(libc::signal(libc::SIGTTIN, action));
        stry!(libc::signal(libc::SIGTTOU, action));
    }
    Ok(())
}

pub fn fork(inter: bool, pgid: Option<Pgid>) -> Result<Option<Pid>> {
    let c_pid = unsafe { etry!(libc::fork()) };

    if c_pid == 0 {
        if inter {
            if let Err(e) = set_signal_ignore(false) {
                warn!("Child could not listen to signals: {}", e);
                exit(2);
            }
        }
        Ok(None)
    } else {
        let pid = Some(Pid(c_pid));
        if inter {
            try!(put_into_pgrp(pid, pgid));
            try!(set_signal_ignore(false));
        }

        Ok(pid)
    }
}

/// Puts a process into a process group.
/// If pid is None, puts the calling process into the process group.
/// If pgid is None, creates a new process group with pgid = pid.
///
/// Returns, upon success, the Pgid into which this process was put.
///
/// (Even if both pid and pgid is None, returns a nonzero Pgid value which
/// can be used to put other processes into the same process group.)
pub fn put_into_pgrp(pid: Option<Pid>, pgid: Option<Pgid>) -> Result<Pgid> {
    let pid = unsafe {
        if let Some(pid) = pid {
            pid.0
        } else {
            libc::getpid()
        }
    };
    if let Some(pgid) = pgid {
        unsafe {
            etry!(libc::setpgid(pid, pgid.0));
        }
        Ok(pgid)
    } else {
        unsafe {
            etry!(libc::setpgid(pid, pid));
        }
        Ok(Pgid(pid))
    }
}

pub fn give_terminal(pgid: Pgid) -> Result<()> {
    unsafe {
        if etry!(libc::tcgetpgrp(libc::STDIN_FILENO)) != pgid.0 {
            etry!(tcsetpgrp(libc::STDIN_FILENO, pgid.0));
        }
    }
    Ok(())
}

pub fn take_terminal() -> Result<()> {
    unsafe {
        let my_pgid = Pgid(libc::getpgrp());
        give_terminal(my_pgid)
    }
}

// This shouldn't probably do much, since we want BuiltinProcesses to work correctly
// in the interactive case, and those won't be calling this function at all.
pub fn execv(cmd: *const i8, argv: *const *const i8) -> Error {
    unsafe {
        libc::execv(cmd, argv);
    }

    Error::last_os_error()
}
