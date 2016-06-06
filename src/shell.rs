use prompt::Prompt;
use prompt::LineState;
use sym::Symtable;
use hist::Histvec;

use exec::Job;
use eval;

/// terrible God object to make state accessible to everyone everywhere
pub struct Shell {
    pub jobs: Vec<Job>,

    pub pr: Box<Prompt>,
    pub ls: LineState,
    pub st: Symtable,
    pub ht: Histvec
}

impl Shell {
    fn exec_mcl(&mut self, mut job: Job, collect: bool) -> Option<String> {
        if collect {
            job.do_pipe_out = true;
        }
        job.spawn(self);
        let fg = job.fg;
        if collect {
            let r = job.wait_collect();
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
            if let Some(status) = job.wait() {
                self.st.set("_?", status.to_string());
            }
        }
    }

    pub fn exec(&mut self, job: Job) {
        self.exec_mcl(job, false);
    }

    pub fn exec_collect(&mut self, job: Job) -> String {
        self.exec_mcl(job, true).unwrap()
    }

    fn get_line(&mut self, in_lines: &mut Option<Vec<String>>) -> Option<String> {
        if let &mut Some(ref mut v) = in_lines {
            if v.len() > 0 {
                Some(v.remove(0))
            } else {
                None
            }
        } else {
            match self.pr.prompt(&self.ls, &self.st, &mut self.ht) {
                Some(Ok(pr_in)) => Some(pr_in),
                Some(Err(e))    => {
                    err!("Couldn't get input: {}", e);
                    panic!("");
                },
                None => None
            }
        }
    }

    pub fn input_loop(&mut self, mut in_lines: Option<Vec<String>>) {
        // saves previous inputs in the case of LineState::Continue
        let mut input_buf = String::new();
        // saves "future" inputs in the case of multi-line input
        let mut next_buf: Option<String> = None;
        loop {
            // get input
            // "input" is the new input we are adding in this loop
            let input;

            if let Some(new_buf) = next_buf {
                // we have new things to deal with without prompting for more
                let (spl_input, spl_next_buf) = eval::spl_line(&new_buf);
                input = spl_input;
                next_buf = spl_next_buf;
            } else {
                match self.get_line(&mut in_lines) {
                    Some(prompt_in) => {
                        // we needed more and we got more
                        let (spl_input, spl_next_buf) = eval::spl_line(&prompt_in);
                        input = spl_input;
                        next_buf = spl_next_buf;
                    },
                    None => {
                        break;
                    }
                }
            }

            // do stuff with input
            self.ls = match self.ls {
                LineState::Comment => {
                    if input.trim() == "###" {
                        input_buf = String::new();
                        LineState::Normal
                    } else {
                        LineState::Comment
                    }
                },
                LineState::Normal | LineState::Continue => {
                    input_buf.push_str(&input);

                    let (t_job, ls) = eval::eval(self, input_buf.clone());
                    
                    if ls == LineState::Normal {
                        if let Some(t_job) = t_job {
                            self.ht.hist_add (input_buf.trim());
                            self.exec(t_job);
                        }
                        input_buf = String::new();
                    }

                    ls
                },
            };
        }

        if self.ls != LineState::Normal {
            // warn or info?
            warn!("Line state left non-normal");
            self.ls = LineState::Normal;
        }
    }
}
