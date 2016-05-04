use sym;
use shell;

extern crate unicode_segmentation;
use self::unicode_segmentation::UnicodeSegmentation;

use std::path::PathBuf;

use prompt::LineState;
use err::warn;
use exec::Job;
use exec::Process;
use exec::BuiltinProcess;
use exec::BinProcess;

#[derive(PartialEq)]
enum TokenType {
    Word,
    Paren,
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

fn word_boundary<'a, F>(cmd: &'a str, i: usize, ctok_start: usize,
                        quot: Qu, pdepth: usize, bdepth: usize,
                        ttype: TokenType,
                        res: &mut Vec<(Option<&'a str>, TokenType)>,
                        tok_cl: F,
                        ) -> usize 
        where F: Fn(&mut Vec<(Option<&str>, TokenType)>) {

    if quot == Qu::None && pdepth == 0 && bdepth == 0 {
        if i > ctok_start { // need to add a token
            let wd = &cmd[ctok_start..i];
            res.push((Some(wd), ttype));
        }

        tok_cl(res);
        i + 1           // FIXME: c.len() might not be 1
    } else {
        ctok_start
    }
}

fn tokenize<'a> (cmd: &'a str)
        -> Result<Vec<(Option<&'a str>, TokenType)>, TokenException> {

    let mut res: Vec<(Option<&str>, TokenType)> = Vec::new();

    let mut ctok_start: usize = 0;

    let mut pflag = false;     // if this token has had parens

    let mut pdepth: usize = 0; // paren depth
    let mut bdepth: usize = 0; // brace-block depth
    let mut quot = Qu::None;   // quote type
    let mut bs = false;        // backslash

    // TODO: testing to see if counting matchable tokens beforehand improves
    // performance ... probably won't

    for (i, c) in cmd.grapheme_indices(true) {
        if bs { // the last character was a backspace
            bs = false;
            continue;
        }

        if c.trim().is_empty() { // if c is whitespace
            let ttype = if pflag { TokenType::Paren } else { TokenType::Word };
            let new_cts = word_boundary(cmd, i, ctok_start, quot, pdepth, bdepth,
                                       ttype, &mut res, |_r| { });
            if new_cts != ctok_start {
                pflag = false;
                ctok_start = new_cts;
            }
            continue;
        }

        match c {
            // pipe
            "|"  => {
                let ttype = if pflag { TokenType::Paren } else { TokenType::Word };
                ctok_start = word_boundary(cmd, i, ctok_start, quot, pdepth, bdepth,
                                           ttype, &mut res, |r| {
                                               r.push((None, TokenType::Pipe));
                                           });
                pflag = false;
            },
            
            // quote/backslash boundaries
            "\"" => {
                quot = match quot {
                    Qu::None   => Qu::Double,
                    Qu::Double => Qu::None,
                    Qu::Single => Qu::Single
                };
            },
            "'"  => {
                quot = match quot {
                    Qu::None   => Qu::Single,
                    Qu::Single => Qu::None,
                    Qu::Double => Qu::Double
                };
            },
            "\\" => {
                bs = true;
            }
            
            // paren boundaries
            // don't automatically end the token at the last ')'
            "("  => {
                if quot != Qu::Single {
                    pflag = true;
                    pdepth = pdepth + 1;
                }
            },
            ")"  => {
                if quot != Qu::Single {
                    if pdepth > 0 {
                        pdepth = pdepth - 1;
                    } else {
                        return Err(TokenException::ExtraRightParen);
                    }
                }
            },

            // block boundaries
            // *do* automatically end the token at the last '}'
            "{"  => {
                if quot == Qu::None && pdepth == 0 {
                    if bdepth == 0 {
                        let ttype = if pflag { TokenType::Paren } 
                                    else { TokenType::Word };
                        ctok_start = word_boundary(cmd, i, ctok_start, quot, pdepth,
                                                   bdepth, ttype,
                                                   &mut res, |_r| { });
                    }
                    bdepth = bdepth + 1;
                }
            },
            "}"  => {
                if quot == Qu::None && pdepth == 0 {
                    if bdepth > 1 {
                        bdepth = bdepth - 1;
                    } else if bdepth == 1 {
                        bdepth = bdepth - 1;
                        ctok_start = word_boundary(cmd, i, ctok_start, quot, pdepth,
                                                   bdepth, TokenType::Block,
                                                   &mut res, |_r| { }); 
                    } else {
                        return Err(TokenException::ExtraRightBrace);
                    }
                }
            },
            // TODO: line blocks are hard
            
            // nothing else matters
            _    => { }
        }
    }

    // throw in the last word
    let ttype = if pflag { TokenType::Paren } else { TokenType::Word };
    word_boundary(cmd, cmd.len(), ctok_start, quot, pdepth, bdepth,
                  ttype, &mut res, |_r| { });

    // if this line ain't done
    if bs || quot != Qu::None || pdepth > 0 || bdepth > 0 {
        return Err(TokenException::Incomplete);
    }

    Ok(res)
}

// Eats quotes & backslashes && resolves parens
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
                    Qu::Double => Qu::Double,
                    Qu::Single => Qu::None
                };
            },
            "\"" => {
                quot = match quot {
                    Qu::None   => Qu::Double,
                    Qu::Single => Qu::Single,
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
                let sym_res = match sh.st.resolve_types(&sym_n,
                                        Some(vec![sym::SymType::Var,
                                                  sym::SymType::Environment])) {
                    Some(sym::Sym::Var(s)) => s,
                    Some(sym::Sym::Environment(s)) => s,
                    None => {
                        // TODO: __tin_undef_fail
                        "".to_string()
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

pub fn eval(sh: &mut shell::Shell, cmd: String) -> (Option<Job>, LineState) {
    let cmd = cmd.trim().to_string();

    if cmd == "###" {
        return (None, LineState::Comment);
    }

    let mut job = Job::new(cmd.clone());
    let mut cproc: Box<Process> = Box::new(BuiltinProcess::default());
   
    // TODO: handle aliasing HERE, BEFORE tokenize
    //  - split on end of first word into [word][cmd_remainder]
    //      - if tokenize() makes an Iterator we can just do
    //        cmd.tokenize().nth(1).unwrap()
    //        to get what we want
    //  - if first word is a var, reassign cmd to be
    //      [resolved_word][cmd_remainder]
    //
    // This avoids awkward recursion, and allows for complex
    // structures in aliases like flow control && 'with' clauses.

    // TODO: make tokenize() return an Iterator of Result<...>s.
    // Then, on an Incomplete, we can ``cache'' our already-okay
    // evaluated results to reduce redundancy. For large block-based
    // scripts && functions this should improve performance greatly
    match tokenize(&cmd) {
        Ok(tokens) => for (tok, ttype) in tokens {
            match ttype {
                TokenType::Word | TokenType::Paren => {
                    let tok = tok_resolve(sh, tok.unwrap());
                    if !cproc.has_args() {
                        // first word -- convert process to appropriate thing
                        cproc = match sh.st.resolve_types(&tok,
                                                  Some(vec![sym::SymType::Binary,
                                                       sym::SymType::Builtin])) {
                            Some(sym::Sym::Builtin(b)) =>
                                Box::new(BuiltinProcess::new(&b)),
                            Some(sym::Sym::Binary(b))  =>
                                Box::new(BinProcess::new(&b)),
                            None =>
                                Box::new(BinProcess::new(&PathBuf::from(tok.clone()))),
                            _ =>
                                unreachable!()
                        };
                    } else {
                        cproc.push_arg(tok);
                    }
                },
                TokenType::Pipe  => {
                    job.procs.push(cproc);
                    cproc = Box::new(BuiltinProcess::default());
                },
                TokenType::Redir => {
                    // TODO: convert into probably a Redir struct
                    // and attach to process.
                    println!("redir: {}", tok.unwrap())
                },
                TokenType::Block => {
                    // TODO: convert block to a Vec<String> of lines.
                    // insert block into the argument vector.
                    println!("block: {}", tok.unwrap())
                }
            }
        },
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
    job.procs.push(cproc);

    (Some(job), LineState::Normal)
}
