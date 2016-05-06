use prompt::Prompt;
use prompt::LineState;
use sym::Symtable;
use hist::Histvec;
use err::err;

extern crate libc;

// #[link(name = "unistd")]
extern {
    fn tcsetpgrp(fd: libc::c_int, pgrp: libc::pid_t) -> libc::c_int;
}

/// terrible God object to make state accessible to everyone everywhere
pub struct Shell {
    pub interactive: bool,

    pub pr: Box<Prompt>,
    pub ls: LineState,
    pub st: Symtable,
    pub ht: Histvec
}

pub fn interactive_init() {
    unsafe {
/*             // wait until we are fg
        while libc::tcgetpgrp(libc::STDIN_FILENO) != shell_pgid {
            libc::kill(-shell_pgid, libc::SIGTTIN);
        } */

        // /*
        // ignore interactive & job-control signals
        libc::signal(libc::SIGINT,  libc::SIG_IGN);
        libc::signal(libc::SIGQUIT, libc::SIG_IGN);
        libc::signal(libc::SIGTSTP, libc::SIG_IGN);
        libc::signal(libc::SIGTTIN, libc::SIG_IGN);
        libc::signal(libc::SIGTTOU, libc::SIG_IGN);
        // */

        // put ourselves in a new process group
        /* shell_pgid = libc::getpid();
        if libc::setpgid(shell_pgid, shell_pgid) < 0 {
            err("Couldn't put shell in its own process group");
        }

        // take the terminal!!!
        if tcsetpgrp(libc::STDIN_FILENO, shell_pgid) < 0 {
            err("Couldn't control the terminal.");
        } */
    }
}


/// Put that terminal where it came from or so help me
/// To be used by both parent && child processes
fn interactive_exec(pid: i32, mut pgid: i32) -> i32 {
    if pgid == 0 {
        pgid = pid;
    }

    pgid
}


pub fn parent_exec(fg: bool, pid: i32, mut pgid: i32) {
    /* 
    pgid = interactive_exec(pid, pgid);

    unsafe {
        if fg {
            if tcsetpgrp(libc::STDIN_FILENO, pgid) < 0 {
                err("Could not give terminal to child");
            }
        }
    } */
}


/// Put that terminal where it came from or so help me
/// Used only by child process
pub fn subproc_exec(mut pgid: i32) {
    /* unsafe {
        let pid = libc::getpid();
        pgid = interactive_exec(pid, pgid);

        if libc::setpgid (pid, pgid) < 0 {
            err("Could not set child process group");
        }

        libc::signal(libc::SIGINT,  libc::SIG_DFL);
        libc::signal(libc::SIGQUIT, libc::SIG_DFL);
        libc::signal(libc::SIGTSTP, libc::SIG_DFL);
        libc::signal(libc::SIGTTIN, libc::SIG_DFL);
        libc::signal(libc::SIGTTOU, libc::SIG_DFL);
    } */
}
