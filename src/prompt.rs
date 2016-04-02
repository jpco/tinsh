#![allow(dead_code)]

use std::io::prelude::*;
use std::io;
use std::str;
use std::env;
use std::fs::File;
use std::os::unix::io::AsRawFd;

use sym::Symtable;
use hist::Histvec;

extern crate termios;
use self::termios::*;

pub enum LineState {
    Normal,
    Comment
}

pub trait Prompt {
    fn prompt(&mut self, ls: &LineState, st: &Symtable, ht: &mut Histvec) -> String;
}

pub struct BasicPrompt;
impl Prompt for BasicPrompt {
    fn prompt(&mut self, _ls: &LineState, _st: &Symtable, _ht: &mut Histvec) -> String {
        print!("$ ");
        io::stdout().flush().ok().expect("Could not flush stdout");

        let mut buf = String::new();
        io::stdin().read_line(&mut buf)
            .expect("Could not read stdin");

        buf
    }
}

pub struct StdPrompt {
    term_fi: File,
    termios: Termios,
    in_tcflag: tcflag_t,
    out_tcflag: tcflag_t,

    ansi: String,
    buf: String,
    prompt_l: usize,
    idx: usize
}

impl StdPrompt {
    pub fn init() -> StdPrompt {
        // create prompt && termios
        let mut pr = match File::open("/dev/tty") {
            Ok(fi) => StdPrompt {
                termios: Termios::from_fd(fi.as_raw_fd()).unwrap(),
                term_fi: fi,
                in_tcflag: 0,
                out_tcflag: 0,
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

    fn print_prompt(&mut self, ls: &LineState) {
        match *ls {
            LineState::Normal => {
                let prompt_str = env::var("PWD").unwrap_or("".to_string());
                self.prompt_l = prompt_str.len() + 3;
                print!("{}$ ", prompt_str);
            },
            LineState::Comment => {
                self.prompt_l = 5;
                print!("### ");
            }
        }
        io::stdout().flush().ok().expect("Could not flush stdout");
    }

    fn get_char(&self) -> Option<char> {
        let mut chrbuf: Vec<u8> = Vec::new();

        // this whole thing is a nightmare, but there's no 
        // reliable io::stdin().chars() so here we are
        for chres in io::stdin().bytes() {
            match chres {
                Ok(ch) => {
                    // println!("[31m{}[0m", ch as u8);
                    chrbuf.push(ch);
                    if let Ok(chr) = str::from_utf8(&chrbuf) {
                        return chr.chars().next();
                    }
                },
                Err(e) => {
                    println!("{}", e);
                    return None;
                }
            }
        }

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

    fn interp(&mut self, input: &str, st: &Symtable, ht: &mut Histvec) -> bool { 
        match input {
            "\n" => return true,
            "\t" => { /* tab completion */ },
            "[3~" => if self.buf.len() > 0 && self.idx < self.buf.len() {
                self.buf.remove(self.idx);
                self.reprint();              
            },
            "[A" => if let Some(nl) = ht.hist_up() {
                self.buf = nl.to_string();
                if self.buf.len() < self.idx {
                    self.idx = self.buf.len();
                }
                self.reprint();
            },
            "[B" => if let Some(nl) = ht.hist_down() {
                self.buf = nl.to_string();
                if self.buf.len() < self.idx {
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
            _ => {
                // this is bad but the case where it is bad would also be bad
                // on the part of the user... so we'll call it a draw for now.
                for ch in input.chars() {
                    self.buf.insert(self.idx, ch);
                    self.idx += 1;
                }
                self.reprint();
            }
        }

        false
    }
}

impl Prompt for StdPrompt {
    // functionality to add:
    //  - tab completion
    //  - proper POSIX terminal control
    //  - coloration (?)
    //  - history
    //  - ANSI code interpretation
    fn prompt(&mut self, ls: &LineState, st: &Symtable, ht: &mut Histvec) -> String {
        self.print_prompt(ls);
        self.prep_term();

        self.buf.clear();
        self.idx = 0;

        while let Some(ch) = self.get_char() {
            if let Some(res) = self.ansi(ch) {
                if self.interp(&res, st, ht) { break; }
            }
        }

        println!("");
        self.unprep_term();

        self.buf.clone()
    }
}

impl Drop for StdPrompt {
    fn drop(&mut self) {
        self.unprep_term();
    }
}

// todo: FilePrompt
