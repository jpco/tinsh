extern crate unicode_segmentation;
use self::unicode_segmentation::UnicodeSegmentation;

extern crate regex;
use self::regex::Regex;

#[derive(PartialEq)]
pub enum TokenType {
    Word,
    Pipe,
    Redir,
    Block
}

pub enum TokenException {
    ExtraRightParen,
    ExtraRightBrace,
    Incomplete
}

#[derive(PartialEq, Clone, Copy)]
pub enum Qu {
    None,
    Single,
    Double
}

pub struct Lexer {
    complete: String,
    ctok_start: usize
}

impl Lexer {
    fn new(line: String) -> Self {
        Lexer {
            complete: line,
            ctok_start: 0
        }
    }

    // these next two methods are for re-tokenizing after a Continue
    fn get_start(&self) -> usize {
        self.ctok_start
    }
}

pub fn lex(s: String) -> Lexer {
    Lexer::new(s)
}

fn build_word(tok: String) -> (Option<String>, TokenType) {
    let r_syntax = Regex::new(r"^(~>|-(\d+|&)?>(\d+|\+)?)$").unwrap();
    let l_syntax = Regex::new(r"^((\d+)?<(<|\d+)?-|<~)$").unwrap();

    let ttype = if r_syntax.is_match(&tok) || l_syntax.is_match(&tok) {
        TokenType::Redir
    } else {
        TokenType::Word
    };

    (Some(tok), ttype)
}

impl Iterator for Lexer {
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
                            let res = self.complete[self.ctok_start+1..i].to_string();
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
