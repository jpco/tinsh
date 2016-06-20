#![allow(dead_code)]

use std::io::prelude::*;
use std::io;
use std::io::Result;
use std::str;
use std::fs::File;
use std::os::unix::io::AsRawFd;

use compl::complete;

extern crate termios;
use self::termios::*;

use shell::Shell;
use sym;

#[derive(PartialEq, Clone, Copy)]
pub enum LineState {
    Normal,
    Comment,
    Continue
}

// None = EOF
// Err(e) = obvious
pub trait Prompt {
    fn prompt(&mut self, sh: &mut Shell) -> Option<Result<String>>;
}

// This may have been obviated by input_loop
/// CmdPrompt: basic prompt which supplies a single command specified at creation.
pub struct CmdPrompt {
    cmd: Option<String>
}
impl CmdPrompt {
    pub fn new(cmd: String) -> Self {
        CmdPrompt {
            cmd: Some(cmd)
        }
    }
}

impl Prompt for CmdPrompt {
    fn prompt(&mut self, _sh: &mut Shell) -> Option<Result<String>> {
        if self.cmd.is_some() {
            let cmd = self.cmd.clone();
            self.cmd = None;

            Some(Ok(cmd.unwrap()))
        } else {
            None
        }
    }
}

/// BasicPrompt: the simplest stdin prompt.
pub struct BasicPrompt;

impl Prompt for BasicPrompt {
    fn prompt(&mut self, _sh: &mut Shell) -> Option<Result<String>> {
        let mut buf = String::new();
        let res = io::stdin().read_line(&mut buf);
        if let Err(e) = res {
            Some(Err(e))
        } else {
            let x = res.unwrap();
            if x == 0 {
                None
            } else {
                Some(Ok(buf))
            }
        }
    }
}

/// FilePrompt: read the lines from a file, woohoo.
pub struct FilePrompt {
    br: io::BufReader<File>
}

impl FilePrompt {
    pub fn new(file: &str) -> Result<Self> {
        let f = try!(File::open(file));

        Ok(FilePrompt {
            br: io::BufReader::new(f)
        })
    }
}

impl Prompt for FilePrompt {
    fn prompt(&mut self, _sh: &mut Shell) -> Option<Result<String>> {
        let mut line = String::new();
        match self.br.read_line(&mut line) {
            Ok(n) => { 
                if n == 0 { return None; }
            },
            Err(e) => {
                return Some(Err(e));
            }
        };

        Some(Ok(line))
    }
}


/// StdPrompt: the standard interactive prompt of the Tin shell.
/// Supports all manner of interactive goodness -- coloration, tab completion, etc.
pub struct StdPrompt {
    term_fi: File,
    termios: Termios,
    in_tcflag: tcflag_t,
    out_tcflag: tcflag_t,

    print_multi: bool,
    ls: LineState,

    ansi: String,
    buf: String,
    prompt_l: usize,
    idx: usize
}

impl StdPrompt {
    pub fn new() -> Self {
        // create prompt && termios
        let mut pr = match File::open("/dev/tty") {
            Ok(fi) => StdPrompt {
                termios: Termios::from_fd(fi.as_raw_fd()).unwrap(),
                term_fi: fi,
                in_tcflag: 0,
                out_tcflag: 0,
                ls: LineState::Normal,
                print_multi: false,
                ansi: String::new(),
                buf: String::new(),
                prompt_l: 0,
                idx: 0
            },
            Err(_) => panic!("Could not open tty.")
        };

        // initialize termios settings
        pr.out_tcflag = pr.termios.c_lflag;
        pr.in_tcflag = pr.out_tcflag & !(ECHO | ECHONL | ICANON | IEXTEN);

        // return
        pr
    }

    fn prep_term(&mut self) {
        self.termios.c_lflag = self.in_tcflag;
        tcsetattr(self.term_fi.as_raw_fd(), TCSAFLUSH, &self.termios).unwrap();
    }

    fn unprep_term(&mut self) {
        self.termios.c_lflag = self.out_tcflag;
        tcsetattr(self.term_fi.as_raw_fd(), TCSAFLUSH, &self.termios).unwrap();
    }

    fn cpos(&mut self) -> (u8, u8) {
        // get terminal settings correct
        let cf = self.termios.c_lflag;
        self.termios.c_lflag = cf & !(ECHO | CREAD | ICANON);
        tcsetattr(self.term_fi.as_raw_fd(), TCSAFLUSH, &self.termios).unwrap();

        // write terminal request
        // FIXME: BETTER ERRORS
        if let Err(e) = io::stdout().write("[6n".as_bytes()) {
            println!("{}", e);
            panic!();
        }
        if io::stdout().flush().is_err() { panic!(); }
        // read
        let mut r = [0; 10];
        if io::stdin().read(&mut r).is_err() { panic!(); }
        if r.len() < 6 || r[0] != 27 || r[1] != 91 { panic!(); }

        let mut sc = false;
        let mut x = 0;
        let mut y = 0;
        for &c in r.iter().skip(2) {
            if 0x30 <= c && c < 0x3A { // a digit
                if sc {
                    y = y * 10 + (c - 0x30);
                } else {
                    x = x * 10 + (c - 0x30);
                }
            } else if c == 0x3B {  // ;
                if !sc { sc = true; }
                else { panic!(); }
            } else if c == 0x52 {  // R
                if sc { break; }
                else { panic!(); }
            } else { println!("ack! {}", c); panic!(); }
        } 

        // return settings to normal
        self.termios.c_lflag = cf;
        tcsetattr(self.term_fi.as_raw_fd(), TCSAFLUSH, &self.termios).unwrap();

        (x, y)
    }

    fn print_prompt(&mut self, sh: &mut Shell) {
        print!("[G");
        io::stdout().flush().ok().expect("Could not flush stdout");

        let pr_name = match self.ls {
            LineState::Normal => "_prompt",
            LineState::Comment => "_prompt_comment",
            LineState::Continue => "_prompt_continue"
        };
        match sh.st.resolve_varish(pr_name) {
            Some(sym::SymV::Var(s)) | Some(sym::SymV::Environment(s)) => {
                print!("{}", s)
            },
            None => {
                let os = sh.status_code;
                sh.input_loop(Some(vec![pr_name.to_owned()]), false);
                sh.status_code = os;
            }
        }

        io::stdout().flush().ok().expect("Could not flush stdout");
        let (_, y) = self.cpos();
        self.prompt_l = y as usize;
    }

    fn get_char(&self) -> Option<Result<char>> {
        let mut chrbuf: Vec<u8> = Vec::new();

        // this whole thing is a nightmare, but there's no 
        // reliable io::stdin().chars() so here we are
        for chres in io::stdin().bytes() {
            match chres {
                Ok(ch) => {
                    if ch as u8 == 4 { // EOF
                        return None;
                    }
                    // TODO: this is broken!! we need to see if the next
                    // char is an ok char to start with before saying this one
                    // is ok... UTF-8 is a bit of a nightmare
                    chrbuf.push(ch);
                    if let Ok(chr) = str::from_utf8(&chrbuf) {
                        return Some(Ok(chr.chars().next().unwrap()));
                    }
                },
                Err(e) => {
                    return Some(Err(e));
                }
            }
        }

        // if we're *here*, we've hit EOF, and need to handle that case.
        None
    }

    fn ansi(&mut self, ch: char) -> Option<String> {
        let len = self.ansi.len();
        if len == 0 && ch == '' {
            self.ansi.push(ch);
            return None;
        } else if len == 1 && ch == '[' {
            self.ansi.push(ch);
            return None;
        } else if len >= 2 && ((ch as u8) < 64 || (ch as u8) > 126) {
            self.ansi.push(ch);
            return None;
        }

        let mut ret = self.ansi.clone();
        ret.push(ch);
        self.ansi.clear();

        Some(ret)
    }

    fn reprint_cursor(&self) {
        print!("[{}G", self.prompt_l + self.idx);
        io::stdout().flush().ok().expect("Could not flush stdout");
    }

    fn reprint(&self) {
        print!("[{}G{}[K", self.prompt_l, self.buf);
        self.reprint_cursor();
    }

    fn interp(&mut self, input: &str, sh: &mut Shell) -> bool {
        if input != "\t" { self.print_multi = false; }
        match input {
            "\n" => return true,
            "\t" => {
                let bufclone = self.buf.clone();
                let (pre, post) = bufclone.split_at(self.idx);
                let (pre, wd_pre) = pre.split_at(match pre.rfind(char::is_whitespace) {
                        Some(n) => n + 1,
                        None    => 0
                    });
                let new_pre = complete(&wd_pre, &sh.st, pre.is_empty(), self.print_multi);
                self.buf = pre.to_string();
                self.buf.push_str(&new_pre);
                self.buf.push_str(post);
                self.idx = pre.len() + new_pre.len();
                self.print_multi = true;
                // TODO: get rid of this when we improve interactive printing
                self.print_prompt(sh); // in case complete() printed options
                self.reprint();
            },
            "[3~" => if self.buf.len() > 0 && self.idx < self.buf.len() {
                self.buf.remove(self.idx);
                self.reprint();              
            },
            "[A" => if let Some(nl) = sh.ht.hist_up() {
                let at_end = self.idx == self.buf.len();
                self.buf = nl.to_string();
                if at_end || self.buf.len() < self.idx {
                    self.idx = self.buf.len();
                }
                self.reprint();
            },
            "[B" => if let Some(nl) = sh.ht.hist_down() {
                let at_end = self.idx == self.buf.len();
                self.buf = nl.to_string();
                if at_end || self.buf.len() < self.idx {
                    self.idx = self.buf.len();
                }
                self.reprint();
            },
            "[C" => if self.idx < self.buf.len() {
                self.idx += 1;
                self.reprint_cursor();
            },
            "[D" => if self.idx > 0 {
                self.idx -= 1;
                self.reprint_cursor();
            },
            "\x7F" => if self.buf.len() > 0 && self.idx > 0 {
                self.buf.remove(self.idx - 1);
                self.idx -= 1;
                self.reprint();
            },
            "\u{0001}" => { 
                self.idx = 0;
                self.reprint_cursor();
            },
            "\u{0005}" => {
                self.idx = self.buf.len();
                self.reprint_cursor();
            },
            _ => {
                // this is bad but the case where it is bad would also be bad
                // on the part of the user... so we'll call it a draw for now.
                for ch in input.chars() {
                    // print!("{}-", ch as u8);
                    self.buf.insert(self.idx, ch);
                    self.idx += 1;
                }
                // println!("");
                self.reprint();
            }
        }

        false
    }
}

impl Prompt for StdPrompt {
    // functionality to add:
    //  - proper POSIX terminal control
    //  - coloration (?)
    //  - ANSI code interpretation
    fn prompt(&mut self, sh: &mut Shell) -> Option<Result<String>> {
        self.ls = sh.ls;
        self.print_prompt(sh);
        self.prep_term();

        self.buf.clear();
        self.idx = 0;

        loop {
            match self.get_char() {
                Some(Ok(ch)) => {
                    if let Some(res) = self.ansi(ch) {
                        if self.interp(&res, sh) { break; }
                    }
                },
                Some(Err(e)) => {
                    return Some(Err(e));
                },
                None         => {
                    return None;
                }
            }
        }

        // fix this whole deal
        self.buf.push('\n');
        println!("");

        self.unprep_term();

        Some(Ok(self.buf.clone()))
    }
}

impl Drop for StdPrompt {
    fn drop(&mut self) {
        self.unprep_term();
    }
}
