extern crate unicode_segmentation;
use self::unicode_segmentation::UnicodeSegmentation;

extern crate regex;
use self::regex::Regex;

#[derive(PartialEq, Debug)]
pub enum TokenType {
    Word(String),
    Redir(String),
    Pipe,
    Block(String)
}

// TODO: lazy_static! these
fn build_word(tok: String) -> TokenType {
    let r_syntax = Regex::new(r"^(~>|-(\d+|&)?>(\d+|\+)?)$").unwrap();
    let l_syntax = Regex::new(r"^((\d+)?<(<|\d+)?-|<~)$").unwrap();

    if r_syntax.is_match(&tok) || l_syntax.is_match(&tok) {
        TokenType::Redir(tok)
    } else {
        TokenType::Word(tok)
    }
}

#[derive(Debug)]
pub enum TokenException {
    ExtraRightParen,
    ExtraRightBrace,
    Incomplete(LexerState, String)  // the string is the incomplete token
}

#[derive(PartialEq, Clone, Copy, Debug)]
pub enum Qu {
    None,
    Single,
    Double
}

#[derive(Clone, Debug)]
pub struct LexerState {
    pub pdepth: usize,
    pub bdepth: usize,
    pub quot: Qu,
    pub bs: bool
}

impl LexerState {
    fn new() -> Self {
        LexerState {
            pdepth: 0,
            bdepth: 0,
            quot: Qu::None,
            bs: false
        }
    }
}

pub struct Lexer {
    cmd: String,
    off: usize,
    lx: LexerState
}

impl Lexer {
    pub fn new(line: String) -> Self {
        Lexer {
            cmd: line,
            off: 0,
            lx: LexerState::new()
        }
    }

    pub fn with_state(line: String, lx: LexerState) -> Self {
        Lexer {
            cmd: line,
            off: 0,
            lx: lx
        }
    }

    fn is_normal(&self) -> bool {
        self.lx.pdepth == 0 && self.lx.bdepth == 0 && self.lx.quot == Qu::None
    }
}

impl Iterator for Lexer {
    type Item = Result<TokenType, TokenException>;

    fn next(&mut self) -> Option<Result<TokenType, TokenException>> {
        if self.off >= self.cmd.len() && self.is_normal() {
            return None;
        }

        // this is the actual string we want to iterate over
        let tok_str = &self.cmd[self.off..];

        let start_off = self.off;

        for (i, c) in tok_str.grapheme_indices(true) {
            let i = i + start_off;
            if self.lx.bs {
                self.lx.bs = false;
                continue;
            }

            if c.trim().is_empty() { // whitespace
                if self.off == i {
                    self.off += c.len();
                    continue;
                } else if self.is_normal() {
                    let res = self.cmd[self.off..i].to_string();
                    self.off = i+1;
                    return Some(Ok(build_word(res)));
                }
            }

            match c {
                // line comment
                "#" => {
                    if self.is_normal() {
                        if self.off < i {
                            let res = self.cmd[self.off..i].to_string();
                            self.off = i;
                            return Some(Ok(build_word(res)));
                        } else {
                            return None;
                        }
                    }
                }

                // pipe
                "|" => {
                    if self.is_normal() {
                        if self.off < i { // return word before pipe
                            let res = self.cmd[self.off..i].to_string();
                            self.off = i;
                            return Some(Ok(build_word(res)));
                        } else {          // return the pipe itself
                            self.off = i + c.len();
                            return Some(Ok(TokenType::Pipe));
                        }
                    }
                },

                // quotes & backslash
                "\"" => {
                    self.lx.quot = match self.lx.quot {
                        Qu::None   => Qu::Double,
                        Qu::Single => Qu::Single,
                        Qu::Double => Qu::None
                    };
                },
                "'"  => {
                    self.lx.quot = match self.lx.quot {
                        Qu::None   => Qu::Single,
                        Qu::Single => Qu::None,
                        Qu::Double => Qu::Double
                    };
                },
                "\\" => { self.lx.bs = true; },

                // parens
                "("  => {
                    if self.lx.quot != Qu::Single { self.lx.pdepth += 1; }
                },
                ")"  => {
                    if self.lx.quot != Qu::Single {
                        if self.lx.pdepth > 0 {
                            self.lx.pdepth -= 1;
                        } else {
                            return Some(Err(TokenException::ExtraRightParen));
                        }
                    }
                },

                // blocks
                // TODO: line blocks
                "{"  => {
                    if self.lx.quot == Qu::None && self.lx.pdepth == 0 {
                        if self.lx.bdepth == 0 && self.off < i {
                            // need to return last word first
                            let res = self.cmd[self.off..i].to_string();
                            self.off = i;
                            return Some(Ok(build_word(res)));
                        } else {
                            if self.lx.bdepth == 0 { self.off += 1; }
                            self.lx.bdepth += 1;
                            // self.off += 1;
                        }
                    }
                },
                "}"  => {
                    if self.lx.quot == Qu::None && self.lx.pdepth == 0 {
                        if self.lx.bdepth > 1 {
                            self.lx.bdepth -= 1;
                        } else if self.lx.bdepth == 1 {
                            let res = self.cmd[self.off..i].to_string();
                            self.off = i + c.len();
                            self.lx.bdepth -= 1;
                            return Some(Ok(TokenType::Block(res)));
                        } else {
                            return Some(Err(TokenException::ExtraRightBrace));
                        }
                    }
                },

                // nothing else matters
                _    => { }
            }
        }

        let final_ret = if self.is_normal() && self.off < self.cmd.len() {
            let res = self.cmd[self.off..].to_string();
            Some(Ok(build_word(res)))
        } else {
            // if we've gotten this far...
            Some(Err(TokenException::Incomplete(self.lx.clone(),
                                                self.cmd[self.off..].to_string())))
        };

        // enforce that we cannot use this lexer again
        self.off = self.cmd.len();

        final_ret
    }
}
