use posix;
use opts;

use shell::Shell;
use posix::Status;
use posix::ReadPipe;
use posix::WritePipe;
use posix::Pgid;

use exec::Child;

use exec::ProcStruct;
use exec::Process;

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
                if opts::is_set("__tin_inter") {
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

        if self.fg && opts::is_set("__tin_inter") && self.pgid.is_some() {
            if let Err(e) = posix::give_terminal(self.pgid.unwrap()) {
                warn!("Could not give child the terminal: {}", e);
            }
        }

        self.spawned = true;
    }

    pub fn wait(&mut self) -> Option<Status> {
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

        if opts::is_set("__tin_inter") {
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
