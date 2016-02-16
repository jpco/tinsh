#![allow(dead_code)]

use std::io::prelude::*;
use std::io;
use std::str;
use std::fs::File;
use std::os::unix::io::AsRawFd;

extern crate termios;
use self::termios::*;

pub enum LineState {
    Normal,
    Comment
}

pub trait Prompt {
    fn prompt(&mut self, ls: &LineState) -> String;
}

pub struct BasicPrompt;
impl Prompt for BasicPrompt {
    fn prompt(&mut self, _ls: &LineState) -> String {
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

    fn print_prompt(&self, ls: &LineState) {
        match *ls {
            LineState::Normal => print!("$ "),
            LineState::Comment => print!("; ")
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
        } else if len >= 2 && (ch as u8) < 64 && (ch as u8) > 126 {
            self.ansi.push(ch);
            return None;
        }

        let mut ret = self.ansi.clone();
        ret.push(ch);
        self.ansi.clear();
        return Some(ret);
    }

    fn interp(&mut self, input: &str) -> bool {
        match input {
            "\n" => return true,
            "\x7E" => { },
            "[A" => { },
            "[B" => { },
            "[C" => { },
            "[D" => { },
            "\x7F" => {
                if self.buf.pop().is_some() {
                    print!("[D[K");
                    self.idx -= 1;
                    io::stdout().flush().ok().expect("Could not flush stdout");
                }
            },
            _ => {
                // this is bad but the case where it is bad would also be bad
                // on the part of the user... so we'll call it a draw for now.
                for ch in input.chars() {
                    self.buf.insert(self.idx, ch);
                    self.idx += 1;
                }
                print!("{}", input);
                io::stdout().flush().ok().expect("Could not flush stdout");
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
    fn prompt(&mut self, ls: &LineState) -> String {
        self.print_prompt(ls);
        self.prep_term();

        self.buf.clear();
        self.idx = 0;

        while let Some(ch) = self.get_char() {
            if let Some(res) = self.ansi(ch) {
                if self.interp(&res) { break; }
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
