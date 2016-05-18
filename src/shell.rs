use prompt::Prompt;
use prompt::LineState;
use sym::Symtable;
use hist::Histvec;

use exec::Job;

/// terrible God object to make state accessible to everyone everywhere
pub struct Shell {
    pub interactive: bool,

    pub jobs: Vec<Job>,

    pub pr: Box<Prompt>,
    pub ls: LineState,
    pub st: Symtable,
    pub ht: Histvec
}

impl Shell {
    fn exec_mcl(&mut self, mut job: Job, collect: bool) -> Option<String> {
        let int = self.interactive;
        if collect {
            self.interactive = false;
            job.do_pipe_out = true;
        }
        job.spawn(self);
        let fg = job.fg;
        if collect {
            let r = job.wait_collect(self);
            self.interactive = int;
            Some(r)
        } else {
            self.jobs.push(job);
            if fg {
                self.wait();
            }
            None
        }
    }

    // still stubby while we don't have real job control yet
    fn wait(&mut self) {
        if let Some(mut job) = self.jobs.pop() {
            job.wait(self);
        }
    }

    pub fn exec(&mut self, job: Job) {
        self.exec_mcl(job, false);
    }

    pub fn exec_collect(&mut self, job: Job) -> String {
        self.exec_mcl(job, true).unwrap()
    }
}
