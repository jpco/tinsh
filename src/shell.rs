use std::io::Read;

use prompt::Prompt;
use prompt::LineState;
use sym::Symtable;
use hist::Histvec;
use std::process::exit;

use parser;
use posix;
use opts;
use exec::Arg;
use exec::job::Job;

/// terrible God object to make state accessible to everyone everywhere
pub struct Shell {
    pub jobs: Vec<Job>,

    pub status_code: i32,

    pub pr: Box<Prompt>,
    pub ls: LineState,
    pub st: Symtable,
    pub ht: Histvec
}

impl Shell {
    pub fn exec(&mut self, mut job: Job) -> Option<String> {
        job.spawn(self);
        let fg = job.fg;
        self.jobs.push(job);
        if fg {
            self.wait();
        }
        None
    }

    // still stubby while we don't have real job control yet
    fn wait(&mut self) {
        if let Some(mut job) = self.jobs.pop() {
            if let Some(status) = job.wait() {
                self.status_code = status.to_int();
            }
        }
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

    pub fn input_loop(&mut self, mut in_lines: Option<Vec<String>>, hist: bool) {
        // saves previous inputs in the case of LineState::Continue
        let mut input_buf = String::new();
        // saves "future" inputs in the case of multi-line input
        let mut next_buf: Option<String> = None;

        let mut output_buf = String::new();

        loop {
            // get input
            // "input" is the new input we are adding in this loop
            let input;

            if let Some(new_buf) = next_buf {
                // we have new things to deal with without prompting for more
                let (spl_input, spl_next_buf) = parser::spl_line(&new_buf);
                input = spl_input;
                next_buf = spl_next_buf;
            } else {
                match self.get_line(&mut in_lines) {
                    Some(prompt_in) => {
                        // we needed more and we got more
                        let (spl_input, spl_next_buf) = parser::spl_line(&prompt_in);
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
                    let (t_job, ls) = parser::eval(self, input_buf.clone());

                    if ls == LineState::Normal {
                        if let Some(t_job) = t_job {
                            if hist { self.ht.hist_add (input_buf.trim()); }
                            if let Some(out) = self.exec(t_job) {
                                output_buf.push_str(&out);
                            }
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

    pub fn input_loop_collect(&mut self, in_lines: Option<Vec<String>>) -> String {
        let (mut re, wr) = posix::pipe().unwrap();
        match posix::fork(false, None) {
            Err(e) => {
                err!("Could not fork: {}", e);
                "".to_string()
            },
            Ok(None) => {
                self.st.subsh = true;
                re.close();
                if let Err(e) = posix::set_stdout(wr) {
                    err!("Could not set stdout: {}", e);
                }
                opts::set_inter(false);

                self.input_loop(in_lines, false);
                exit(0);
            },
            Ok(Some(ch)) => {
                wr.close();
                let mut output = String::new();
                match re.read_to_string(&mut output) {
                    Ok(_len) => {
                        while output.ends_with('\n') { output.pop(); }
                        output = output.replace('\n', " ");
                    },
                    Err(e) => { warn!("Error reading from child: {}", e) }
                }
                if let Err(e) = posix::wait_pid(&ch) {
                    warn!("Could not wait for child: {}", e);
                }

                output
            }
        }
    }

    pub fn block_exec(&mut self, bv: Vec<String>) -> i32 {
        self.st.new_scope(false);
        self.input_loop(Some(bv), false);
        self.st.del_scope();

        self.status_code
    }

    pub fn line_exec(&mut self, av: Vec<Arg>) -> i32 {
        let mut cmd = String::new();
        for a in av {
            cmd.push_str(&a.into_string());
            cmd.push(' ');
        }

        let (j, ls) = parser::eval(self, cmd);
        if ls == LineState::Normal {
            if let Some(job) = j {
                self.exec(job);
            }
        } else {
            warn!("Could not evaluate passed command.");
        }

        self.status_code
    }
}
