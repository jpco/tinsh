use sym;
use shell;

extern crate unicode_segmentation;
use self::unicode_segmentation::UnicodeSegmentation;

extern crate regex;
use self::regex::Regex;

use std::path::PathBuf;

use sym::SymType;
use sym::Sym;
use prompt::LineState;
use err::warn;
use exec::Job;
use exec::Process;
use exec::BuiltinProcess;
use exec::BinProcess;
use exec::ProcStruct;
use exec::ProcStruct::BinProc;
use exec::ProcStruct::BuiltinProc;

#[derive(PartialEq)]
enum TokenType {
    Word,
    Pipe,
    Redir,
    Block
}

enum TokenException {
    ExtraRightParen,
    ExtraRightBrace,
    Incomplete
}

#[derive(PartialEq, Clone, Copy)]
enum Qu {
    None,
    Single,
    Double
}

struct Tokenizer {
    complete: String,
    ctok_start: usize
}

impl Tokenizer {
    fn new(line: String) -> Self {
        Tokenizer {
            complete: line,
            ctok_start: 0
        }
    }

    // these next two methods are for re-tokenizing after a Continue
    fn get_start(&self) -> usize {
        self.ctok_start
    }

    fn set_start(&mut self, anustrt: usize) {
        self.ctok_start = anustrt;
    }
}

fn tokenize(s: String) -> Tokenizer {
    Tokenizer::new(s)
}

fn build_word(tok: String) -> (Option<String>, TokenType) {
    lazy_static! {
        static ref R_SYNTAX: Regex = Regex::new(r"^(~>|-(\d+|&)?>(\d+|\+)?)$").unwrap();
        static ref L_SYNTAX: Regex = Regex::new(r"^((\d+)?<(<|\d+)?-|<~)$").unwrap();
    }

    let ttype = if R_SYNTAX.is_match(&tok) || L_SYNTAX.is_match(&tok) {
        TokenType::Redir
    } else {
        TokenType::Word
    };

    (Some(tok), ttype)
}

impl Iterator for Tokenizer {
    type Item = Result<(Option<String>, TokenType), TokenException>;

    fn next(&mut self) -> Option<Result<(Option<String>, TokenType), TokenException>> {
        if self.ctok_start == self.complete.len() {
            return None;
        }

        let tok_offset = self.ctok_start;

        // this is the actual string we want to iterate over
        let tok_str = &self.complete[self.ctok_start..];
 
        // bookkeeping vars
        let mut pdepth: usize = 0;        // paren depth
        let mut bdepth: usize = 0;        // block depth
        let mut quot          = Qu::None; // quote type
        let mut bs            = false;    // backslash

        for (i, c) in tok_str.grapheme_indices(true) {
            let i = i + tok_offset;
            if bs {
                bs = false;
                continue;
            }

            if c.trim().is_empty() { // whitespace
                if self.ctok_start == i {
                    self.ctok_start += c.len();
                    continue;
                } else if pdepth == 0 && bdepth == 0 && quot == Qu::None {
                    let res = self.complete[self.ctok_start..i].to_string();
                    self.ctok_start = i;
                    return Some(Ok(build_word(res)));
                }
            }

            match c {
                // line comment
                "#" => {
                    if pdepth == 0 && bdepth == 0 && quot == Qu::None {
                        if self.ctok_start < i {
                            let res = self.complete[self.ctok_start..i].to_string();
                            self.ctok_start = i;
                            return Some(Ok(build_word(res)));
                        } else {
                            return None;
                        }
                    }
                }

                // pipe
                "|" => {
                    if pdepth == 0 && bdepth == 0 && quot == Qu::None {
                        if self.ctok_start < i { // return word before pipe
                            let res = self.complete[self.ctok_start..i].to_string();
                            self.ctok_start = i;
                            return Some(Ok(build_word(res)));
                        } else {                 // return the pipe itself
                            self.ctok_start = i + c.len();
                            return Some(Ok((None, TokenType::Pipe)));
                        }
                    }
                },

                // quotes & backslash
                "\"" => {
                    quot = match quot {
                        Qu::None   => Qu::Double,
                        Qu::Single => Qu::Single,
                        Qu::Double => Qu::None
                    };
                },
                "'"  => {
                    quot = match quot {
                        Qu::None   => Qu::Single,
                        Qu::Single => Qu::None,
                        Qu::Double => Qu::Double
                    };
                },
                "\\" => { bs = true; },

                // parens
                "("  => {
                    if quot != Qu::Single { pdepth = pdepth + 1; }
                },
                ")"  => {
                    if quot != Qu::Single {
                        if pdepth > 0 {
                            pdepth = pdepth - 1;
                        } else {
                            return Some(Err(TokenException::ExtraRightParen));
                        }
                    }
                },

                // blocks
                // TODO: line blocks
                "{"  => {
                    if quot == Qu::None && pdepth == 0 {
                        if bdepth == 0 && self.ctok_start < i {
                            // need to return last word first
                            let res = self.complete[self.ctok_start..i].to_string();
                            self.ctok_start = i;
                            return Some(Ok(build_word(res)));
                        } else {
                            bdepth += 1;
                        }
                    }
                },
                "}"  => {
                    if quot == Qu::None && pdepth == 0 {
                        if bdepth > 1 {
                            bdepth -= 1;
                        } else if bdepth == 1 {
                            let res = self.complete[self.ctok_start..i].to_string();
                            self.ctok_start = i + c.len();
                            return Some(Ok((Some(res), TokenType::Block)));
                        } else {
                            return Some(Err(TokenException::ExtraRightBrace));
                        }
                    }
                },

                // nothing else matters
                _    => { }
            }
        }

        if pdepth == 0 && bdepth == 0 && quot == Qu::None && self.ctok_start < self.complete.len() {
            let res = self.complete[self.ctok_start..].to_string();
            self.ctok_start = self.complete.len();
            return Some(Ok(build_word(res)));
        }

        // if we've gotten this far...
        Some(Err(TokenException::Incomplete))
    }
}

// resolves a word token into a more useful word token
fn tok_resolve(sh: &mut shell::Shell, tok: &str) -> String {
    let mut res = String::new();
    let mut pstack = Vec::new();

    let mut bs = false;
    let mut quot = Qu::None;

    for c in tok.graphemes(true) {
        if bs { // the last character was a backspace
            bs = false;
            if pstack.is_empty() {
                res.push_str(c);
            }
            continue;
        }

        match c {
            // masks
            "\\" => {
                if quot != Qu::Single { bs = true; }
                else { res.push_str(c); }
            },
            "'"  => {
                quot = match quot {
                    Qu::None   => Qu::Single,
                    Qu::Double => {
                        res.push_str(c);
                        Qu::Double
                    },
                    Qu::Single => Qu::None
                };
            },
            "\"" => {
                quot = match quot {
                    Qu::None   => Qu::Double,
                    Qu::Single => {
                        res.push_str(c);
                        Qu::Single
                    },
                    Qu::Double => Qu::None
                };
            },

            // parens
            "("  => {
                pstack.push(String::new());
            },
            ")"  => {
                // time to resolve something!
                // unmatched parens already handled in tokenize()
                let sym_n = pstack.pop().unwrap();
                let sym_res: String = match sh.st.resolve_types(&sym_n,
                                        Some(vec![sym::SymType::Var,
                                                  sym::SymType::Environment])) {
                    Some(sym::Sym::Var(s)) => s,
                    Some(sym::Sym::Environment(s)) => s,
                    None => {
                        let (sym_job, _) = eval(sh, sym_n);
                        if let Some(sym_job) = sym_job {
                            sh.exec_collect(sym_job)
                        } else {
                            // TODO: __tin_undef_fail
                            "".to_string()
                        }
                    },
                    _ => unreachable!()
                };
                if let Some(mut s) = pstack.pop() { // nested vars
                    s.push_str(&sym_res);
                    pstack.push(s);
                } else {                        // un-nested
                    res.push_str(&sym_res);
                }
            },

            // default
            _    => {
                if let Some(mut s) = pstack.pop() {
                    s.push_str(c);
                    pstack.push(s);
                } else {
                    res.push_str(c);
                }
            }
        }
    }

    res
}

// TODO: this whole function when I'm less tired
pub fn spl_line(cmd: &str) -> (String, Option<String>) {
    (cmd.to_string(), None)
}

pub fn eval(sh: &mut shell::Shell, cmd: String) -> (Option<Job>, LineState) {
    let mut cmd = cmd.trim().to_string();

    if cmd == "###" {
        return (None, LineState::Comment);
    }

    let mut job = Job::new(cmd.clone());
    let mut cproc: Box<ProcStruct> = Box::new(BuiltinProc(BuiltinProcess::default()));

    // begin by evaluating any aliases
    let mut alias_tokenizer = tokenize(cmd.clone());
    if let Some(Ok((Some(w), TokenType::Word))) = alias_tokenizer.nth(0) {
        match sh.st.resolve_types(&w, Some(vec![SymType::Var])) {
            Some(Sym::Var(nw)) => {
                let suffix = cmd[alias_tokenizer.get_start()..].to_string();
                cmd = nw;
                cmd.push_str(&suffix);
            },
            _ => { }
        }
    }


    // TODO: on an Incomplete, we can ``cache'' our already-okay
    // evaluated results to reduce redundancy. For large block-based
    // scripts && functions this should improve performance greatly
    for token_res in tokenize(cmd) {
        match token_res {
            Ok((tok, ttype)) => {
                match ttype {
                    TokenType::Word  => {
                        let tok = tok_resolve(sh, &tok.unwrap());
                        // don't do anything with empty tokens, guh
                        if tok.trim().is_empty() { continue; }

                        if !cproc.has_args() {
                            // first word -- convert process to appropriate thing
                            cproc = match sh.st.resolve_types(&tok,
                                                      Some(vec![sym::SymType::Binary,
                                                           sym::SymType::Builtin])) {
                                Some(sym::Sym::Builtin(b)) =>
                                    Box::new(BuiltinProc(BuiltinProcess::new(b))),
                                Some(sym::Sym::Binary(b))  =>
                                    Box::new(BinProc(BinProcess::new(&tok, b))),
                                None =>
                                    Box::new(BinProc(BinProcess::new(&tok, PathBuf::from(tok.clone())))),
                                _ =>
                                    unreachable!()
                            };
                        } else {
                            cproc.push_arg(tok);
                        }
                    },
                    TokenType::Pipe  => {
                        job.procs.push(cproc);
                        cproc = Box::new(BuiltinProc(BuiltinProcess::default()));
                    },
                    TokenType::Redir => {
                        println!("Redirects are still unimplemented");
                    },
                    TokenType::Block => {
                        println!("Blocks are still unimplemented");
                    }
                }
            },

            // TODO: cache && continue
            Err(e)     => match e {
                TokenException::ExtraRightParen => {
                    warn("Extra right parenthesis found");
                    return (None, LineState::Normal);
                },
                TokenException::ExtraRightBrace => {
                    warn("Extra right brace found");
                    return (None, LineState::Normal);
                },
                TokenException::Incomplete      => {
                    return (None, LineState::Continue);
                }
            }
        }
    }
    job.procs.push(cproc);

    // (None, LineState::Normal)
    (Some(job), LineState::Normal)
}
