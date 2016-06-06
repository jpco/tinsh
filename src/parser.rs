use sym;
use shell;

extern crate unicode_segmentation;
use self::unicode_segmentation::UnicodeSegmentation;

extern crate regex;
use self::regex::Regex;

use std::path::PathBuf;

use prompt::LineState;

use exec::Job;
use exec::Process;
use exec::BuiltinProcess;
use exec::BinProcess;
use exec::ProcStruct;
use exec::ProcStruct::BinProc;
use exec::ProcStruct::BuiltinProc;
use exec::Redir;
use exec::Arg;

use lexer::TokenType;
use lexer::TokenException;
use lexer::lex;

fn p_resolve(sh: &mut shell::Shell, pstmt: String) -> String {
    match sh.st.resolve_types(&pstmt,
                              Some(vec![sym::SymType::Var,
                                        sym::SymType::Environment])) {
        Some(sym::Sym::Var(s)) | Some(sym::Sym::Environment(s)) => s,
        None => sh.input_loop_collect(Some(vec![pstmt])),
        _ => unreachable!()
    }
}

// an enum to indicate the token parser's state
#[derive(Debug, Clone, Copy, PartialEq)]
enum ParseState {
    Normal,
    Paren,
    Dquot,
    Squot,
    Pquot
}

// resolves a word token into a more useful word token
fn tok_parse(sh: &mut shell::Shell, tok: &str) -> String {
    let mut res = String::new();
    let mut pbuf = String::new();
    let mut res_buf: String;
    let mut ps_stack = vec![ParseState::Normal];
    let mut bs = false;
    let mut pctr: usize = 0;
   
    for c in tok.graphemes(true) {
        let to_push = match *ps_stack.last().unwrap() {
            ParseState::Normal => {
                if !bs {
                    match c {
                        "\"" => { ps_stack.push(ParseState::Dquot); None },
                        "'"  => { ps_stack.push(ParseState::Squot); None },
                        "`"  => { ps_stack.push(ParseState::Pquot); Some(c) },
                        "("  => { ps_stack.push(ParseState::Paren); None },
                        "\\" => { None },
                        _    => Some(c)
                    }
                } else { Some(c) }
            },
            ParseState::Paren => {
                if !bs {
                    match c {
                        "(" => { pctr += 1; Some(c) },
                        ")" => {
                            if pctr == 0 {
                                let loc_buf = pbuf;
                                pbuf = String::new();
                                ps_stack.pop();
                                res_buf = p_resolve(sh, loc_buf);
                                Some(&res_buf as &str)
                            } else {
                                pctr -= 1;
                                Some(c)
                            }
                        },
                        "'" => { ps_stack.push(ParseState::Squot); None },
                        "`" => { ps_stack.push(ParseState::Pquot); Some(c) }
                        _ => { Some(c) }
                    }
                } else { Some(c) }
            },
            ParseState::Dquot => {
                if !bs {
                    if c == "\"" {
                        ps_stack.pop();
                        None
                    } else if c == "(" {
                        ps_stack.push(ParseState::Paren);
                        None
                    } else { Some(c) }
                } else {
                    Some(c)
                }
            },
            ParseState::Squot  => {
                if c == "'" && !bs {
                    ps_stack.pop();
                    None
                } else {
                    Some(c)
                }
            },
            ParseState::Pquot  => {
                if c == "`" && !bs {
                    ps_stack.pop();
                }
                Some(c)
            }
        };

        // print!("{:?} - ", ps_stack.last().unwrap());
        if let Some(to_push) = to_push {
            // println!("pushing '{}'", to_push);
            if ps_stack.last().unwrap() == &ParseState::Paren {
                pbuf.push_str(to_push);
            } else {
                res.push_str(to_push);
            }
        } else {
            // println!("no push");
        }

        if c == "\\" {
            bs = true;
        } else {
            bs = false;
        }
    }

    res
}

// TODO: this whole function when I'm less tired
pub fn spl_line(cmd: &str) -> (String, Option<String>) {
    let mut ps_stack = vec![ParseState::Normal];
    let mut bs = false;
    let mut pctr: usize = 0;
    let mut spl = None;
   
    for (i, c) in cmd.grapheme_indices(true) {
        if !bs {
            match *ps_stack.last().unwrap() {
                ParseState::Normal => match c {
                        "\"" => { ps_stack.push(ParseState::Dquot); },
                        "'"  => { ps_stack.push(ParseState::Squot); },
                        "`"  => { ps_stack.push(ParseState::Pquot); },
                        "("  => { ps_stack.push(ParseState::Paren); },
                        ";"  => { spl = Some(i); break; }
                        _    => { }
                },
                ParseState::Paren => match c {
                    "(" => { pctr += 1; },
                    ")" => {
                        if pctr == 0 { ps_stack.pop(); }
                        else { pctr -= 1; }
                    },
                    "'" => { ps_stack.push(ParseState::Squot); },
                    "`" => { ps_stack.push(ParseState::Pquot); },
                    _ => { } 
                },
                ParseState::Dquot => {
                    if c == "\"" { ps_stack.pop(); }
                    else if c == "(" { ps_stack.push(ParseState::Paren); }
                },
                ParseState::Squot => if c == "'" { ps_stack.pop(); },
                ParseState::Pquot  => if c == "`" { ps_stack.pop(); }
            }
        }

        if c == "\\" { bs = true; }
        else { bs = false; }
    }

    if let Some(spl) = spl {
        (cmd[0..spl].to_string(), Some(cmd[spl+1..].to_string()))
    } else {
        (cmd.to_string(), None)
    }
}

// An enum to indicate we are waiting for a redirection token.
enum RedirBuf {
    RdArgOut,
    RdArgIn,
    RdFileOut(i32, bool),
    RdFileIn(i32),
    RdStringIn(i32)
}

fn redir_parse(tok: &str) -> (Option<Redir>, Option<RedirBuf>) {
    match tok {
        "~>"  => (None, Some(RedirBuf::RdArgOut)),
        "<~"  => (None, Some(RedirBuf::RdArgIn)),
        "<<-" => (None, Some(RedirBuf::RdStringIn(0))),
        _ => {
            lazy_static! {
                static ref RD_OUT: Regex
                    = Regex::new(r"^-(&|\d*)>(\+|\d*)$").unwrap();
                static ref RD_IN: Regex
                    = Regex::new(r"^(\d*)<(\d*)-$").unwrap();
            }

            if let Some(caps) = RD_OUT.captures(tok) {
                let src_fd = match caps.at(1).unwrap() {
                    "" => 1,
                    "&" => -2,
                    e  => e.parse::<i32>().unwrap()
                };
                match caps.at(2).unwrap() {
                    "" => (None, Some(RedirBuf::RdFileOut(src_fd, false))),
                    "+" => (None, Some(RedirBuf::RdFileOut(src_fd, true))),
                    e  => {
                        let dest_fd = e.parse::<i32>().unwrap();
                        (Some(Redir::RdFdOut(src_fd, dest_fd)), None)
                    }
                }
            } else if let Some(caps) = RD_IN.captures(tok) {
                let dest_fd = match caps.at(1).unwrap() {
                    "" => 0,
                    e  => e.parse::<i32>().unwrap()
                };
                match caps.at(2).unwrap() {
                    "" => (None, Some(RedirBuf::RdFileIn(dest_fd))),
                    e  => {
                        let src_fd = e.parse::<i32>().unwrap();
                        (Some(Redir::RdFdIn(src_fd, dest_fd)), None)
                    }
                }
            } else {
                unreachable!();  // if we get here I have gravely erred
            }
        }
    }
}

pub fn eval(sh: &mut shell::Shell, cmd: String) -> (Option<Job>, LineState) {
    let cmd = cmd.trim().to_string();

    if cmd == "###" {
        return (None, LineState::Comment);
    }

    let mut job = Job::new(cmd.clone());
    let mut cproc: Box<ProcStruct> = Box::new(BuiltinProc(BuiltinProcess::default()));
    let mut rd_buf = None;


    // TODO: on an Incomplete, we can ``cache'' our already-okay
    // evaluated results to reduce redundancy. For large block-based
    // scripts && functions this should improve performance greatly
    for token_res in lex(cmd) {
        match token_res {
            Ok((tok, ttype)) => {
                match ttype {
                    TokenType::Word  => {
                        let tok = tok_parse(sh, &tok.unwrap());
                        // don't do anything with empty tokens, guh
                        if tok.trim().is_empty() { continue; }

                        // gotta finish the redirect!
                        if rd_buf.is_some() {
                            match rd_buf.unwrap() {
                                RedirBuf::RdArgOut          => {
                                    cproc.push_redir(Redir::RdArgOut(tok));
                                },
                                RedirBuf::RdArgIn           => {
                                    cproc.push_redir(Redir::RdArgIn(tok));
                                },
                                RedirBuf::RdFileOut(fd, ap) => {
                                    cproc.push_redir(Redir::RdFileOut(fd, tok, ap));
                                },
                                RedirBuf::RdFileIn(fd)      => {
                                    cproc.push_redir(Redir::RdFileIn(fd, tok));
                                },
                                RedirBuf::RdStringIn(fd)    => {
                                    cproc.push_redir(Redir::RdStringIn(fd, tok));
                                }
                            }
                            rd_buf = None;
                            continue;
                        }

                        if !cproc.has_args() {
                            // first word -- convert process to appropriate thing
                            // TODO: in non-interactive shells we don't need to
                            // evaluate every binary up-front; make resolution more
                            // piecemeal to speed up startup
                            cproc = match sh.st.resolve_types(&tok,
                                                      Some(vec![sym::SymType::Binary,
                                                           sym::SymType::Builtin])) {
                                Some(sym::Sym::Builtin(b)) =>
                                    Box::new(BuiltinProc(BuiltinProcess::new(b))),
                                Some(sym::Sym::Binary(b))  =>
                                    Box::new(BinProc(BinProcess::new(&tok, b))),
                                None =>
                                    // assume it's in the filesystem but not in PATH
                                    // FIXME: this enables '.' in PATH by default
                                    Box::new(BinProc(BinProcess::new(&tok,
                                                        PathBuf::from(tok.clone())))),
                                _ => unreachable!()
                            };
                        } else {
                            cproc.push_arg(Arg::Str(tok));
                        }
                    },
                    TokenType::Pipe  => {
                        if rd_buf.is_some() {
                            warn!("Syntax error: unfinished redirect.");
                            rd_buf = None;
                        }
                        job.procs.push(cproc);
                        cproc = Box::new(BuiltinProc(BuiltinProcess::default()));
                    },
                    TokenType::Redir => {
                        let tok = tok.unwrap();
                        if rd_buf.is_some() {
                            warn!("Syntax error: unfinished redirect.");
                        }
                        let (rd, rdb) = redir_parse(&tok);
                        rd_buf = rdb;
                        if let Some(rd) = rd {
                            cproc.push_redir(rd);
                        }
                    },
                    TokenType::Block => {
                        if rd_buf.is_some() {
                            warn!("Syntax error: unfinished redirect.");
                            rd_buf = None;
                        }
                        let tokv = tok.unwrap().split('\n').filter_map(|l| {
                            if l.trim().is_empty() { None }
                            else { Some(l.trim().to_string()) }
                        }).collect();
                        cproc.push_arg(Arg::Bl(tokv));
                    }
                }
            },

            // TODO: cache && continue
            Err(e)     => match e {
                TokenException::ExtraRightParen => {
                    warn!("Syntax error: extra right parenthesis found");
                    return (None, LineState::Normal);
                },
                TokenException::ExtraRightBrace => {
                    warn!("Syntax error: extra right brace found");
                    return (None, LineState::Normal);
                },
                TokenException::Incomplete      => {
                    return (None, LineState::Continue);
                }
            }
        }
    }
    if rd_buf.is_some() {
        warn!("Syntax error: unfinished redirect.");
    }

    job.procs.push(cproc);

    // (None, LineState::Normal)
    (Some(job), LineState::Normal)
}
