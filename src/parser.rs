use sym;

extern crate unicode_segmentation;
use self::unicode_segmentation::UnicodeSegmentation;

extern crate regex;
use self::regex::Regex;

use std::env;
use std::mem;

use prompt::LineState;

use shell::Shell;

use exec::job::Job;
use exec::Process;
use exec::builtinprocess::BuiltinProcess;
use exec::binprocess::BinProcess;
use exec::ProcStruct;
use exec::ProcStruct::BinProc;
use exec::ProcStruct::BuiltinProc;
use exec::Redir;
use exec::Arg;

use lexer::TokenType;
use lexer::TokenException;
use lexer::Lexer;
use lexer::LexerState;

fn p_resolve(sh: &mut Shell, mut pstmt: String, ps: &ParseState) -> Vec<String> {
    if pstmt == "?" {
        return vec![sh.status_code.to_string()];
    }

    // TODO: this but more
    let spl = if pstmt.ends_with("!") {
        pstmt.pop();
        true
    } else {
        false
    };

    let res = match sh.st.resolve_varish(&pstmt) {
        Some(sym::SymV::Var(s)) |
        Some(sym::SymV::Environment(s)) => s,
        None => sh.input_loop_collect(Some(vec![pstmt])),
    };

    if spl && *ps == ParseState::Normal {
        // TODO: better
        let r = res.split_whitespace().map(|x| x.to_owned()).collect::<Vec<_>>();
        if r.len() > 0 {
            r
        } else {
            vec!["".to_string()]
        }
    } else {
        vec![res]
    }
}

// an enum to indicate the token parser's state
#[derive(Debug, Clone, Copy, PartialEq)]
enum ParseState {
    Normal,
    Paren,
    Dquot,
    Squot,
    Pquot,
}

// resolves a word token into more useful word tokens
fn tok_parse(sh: &mut Shell, tok: &str) -> Vec<String> {
    let mut res = Vec::new();
    let mut c_res = String::new();
    let mut pbuf = String::new();
    let mut res_buf: String;
    let mut ps_stack = vec![ParseState::Normal];
    let mut bs = false;
    let mut pctr: usize = 0;

    let home_compl = if tok.starts_with('~') {
        match env::var("HOME") {
            Ok(e) => c_res.push_str(&e),
            Err(_) => c_res.push('~'),
        }
        true
    } else {
        false
    };

    for c in tok.graphemes(true).skip(if home_compl {
        1
    } else {
        0
    }) {
        let to_push = match *ps_stack.last().unwrap() {
            ParseState::Normal => {
                if !bs {
                    match c {
                        "\"" => {
                            ps_stack.push(ParseState::Dquot);
                            None
                        }
                        "'" => {
                            ps_stack.push(ParseState::Squot);
                            None
                        }
                        "`" => {
                            ps_stack.push(ParseState::Pquot);
                            Some(c)
                        }
                        "(" => {
                            ps_stack.push(ParseState::Paren);
                            None
                        }
                        "\\" => None,
                        _ => Some(c),
                    }
                } else {
                    Some(c)
                }
            }
            ParseState::Paren => {
                if !bs {
                    match c {
                        "(" => {
                            pctr += 1;
                            Some(c)
                        }
                        ")" => {
                            if pctr == 0 {
                                let loc_buf = pbuf;
                                pbuf = String::new();
                                ps_stack.pop();
                                let mut res_v = p_resolve(sh, loc_buf, ps_stack.last().unwrap());
                                res_buf = res_v.pop().unwrap();
                                if res_v.len() > 0 {
                                    c_res.push_str(&res_v.remove(0));
                                    if !c_res.trim().is_empty() {
                                        res.push(c_res);
                                    }
                                    c_res = String::new();
                                }
                                for a in res_v {
                                    if !a.trim().is_empty() {
                                        res.push(a);
                                    }
                                }
                                Some(&res_buf as &str)
                            } else {
                                pctr -= 1;
                                Some(c)
                            }
                        }
                        "'" => {
                            ps_stack.push(ParseState::Squot);
                            Some(c)
                        }
                        "`" => {
                            ps_stack.push(ParseState::Pquot);
                            Some(c)
                        }
                        _ => Some(c),
                    }
                } else {
                    Some(c)
                }
            }
            ParseState::Dquot => {
                if !bs {
                    if c == "\"" {
                        ps_stack.pop();
                        None
                    } else if c == "(" {
                        ps_stack.push(ParseState::Paren);
                        None
                    } else {
                        Some(c)
                    }
                } else {
                    Some(c)
                }
            }
            ParseState::Squot => {
                if c == "'" && !bs {
                    ps_stack.pop();
                    if ps_stack.contains(&ParseState::Paren) {
                        Some(c)
                    } else {
                        None
                    }
                } else {
                    Some(c)
                }
            }
            ParseState::Pquot => {
                if c == "`" && !bs {
                    ps_stack.pop();
                }
                Some(c)
            }
        };

        if let Some(to_push) = to_push {
            if ps_stack.contains(&ParseState::Paren) {
                pbuf.push_str(to_push);
            } else {
                c_res.push_str(to_push);
            }
        }

        if c == "\\" {
            bs = true;
        } else {
            bs = false;
        }
    }

    if !c_res.trim().is_empty() {
        res.push(c_res);
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
                ParseState::Normal => {
                    match c {
                        "\"" => {
                            ps_stack.push(ParseState::Dquot);
                        }
                        "'" => {
                            ps_stack.push(ParseState::Squot);
                        }
                        "`" => {
                            ps_stack.push(ParseState::Pquot);
                        }
                        "(" => {
                            ps_stack.push(ParseState::Paren);
                        }
                        ";" => {
                            spl = Some(i);
                            break;
                        }
                        _ => {}
                    }
                }
                ParseState::Paren => {
                    match c {
                        "(" => {
                            pctr += 1;
                        }
                        ")" => {
                            if pctr == 0 {
                                ps_stack.pop();
                            } else {
                                pctr -= 1;
                            }
                        }
                        "'" => {
                            ps_stack.push(ParseState::Squot);
                        }
                        "`" => {
                            ps_stack.push(ParseState::Pquot);
                        }
                        _ => {}
                    }
                }
                ParseState::Dquot => {
                    if c == "\"" {
                        ps_stack.pop();
                    } else if c == "(" {
                        ps_stack.push(ParseState::Paren);
                    }
                }
                ParseState::Squot => {
                    if c == "'" {
                        ps_stack.pop();
                    }
                }
                ParseState::Pquot => {
                    if c == "`" {
                        ps_stack.pop();
                    }
                }
            }
        }

        if c == "\\" {
            bs = true;
        } else {
            bs = false;
        }
    }

    if let Some(spl) = spl {
        (cmd[0..spl].to_string(), Some(cmd[spl + 1..].to_string()))
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
    RdStringIn(i32),
}

fn redir_parse(tok: String) -> (Option<Redir>, Option<RedirBuf>) {
    match &tok as &str {
        "~>" => (None, Some(RedirBuf::RdArgOut)),
        "<~" => (None, Some(RedirBuf::RdArgIn)),
        "<<-" => (None, Some(RedirBuf::RdStringIn(0))),
        _ => {
            let rd_out = Regex::new(r"^-(&|\d*)>(\+|\d*)$").unwrap();
            let rd_in = Regex::new(r"^(\d*)<(\d*)-$").unwrap();

            if let Some(caps) = rd_out.captures(&tok) {
                let src_fd = match caps.at(1).unwrap() {
                    "" => 1,
                    "&" => -2,
                    e => e.parse::<i32>().unwrap(),
                };
                match caps.at(2).unwrap() {
                    "" => (None, Some(RedirBuf::RdFileOut(src_fd, false))),
                    "+" => (None, Some(RedirBuf::RdFileOut(src_fd, true))),
                    e => {
                        let dest_fd = e.parse::<i32>().unwrap();
                        (Some(Redir::RdFdOut(src_fd, dest_fd)), None)
                    }
                }
            } else if let Some(caps) = rd_in.captures(&tok) {
                let dest_fd = match caps.at(1).unwrap() {
                    "" => 0,
                    e => e.parse::<i32>().unwrap(),
                };
                match caps.at(2).unwrap() {
                    "" => (None, Some(RedirBuf::RdFileIn(dest_fd))),
                    e => {
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

pub struct Parser {
    job: Job,
    cproc: Box<ProcStruct>,
    rd_buf: Option<RedirBuf>,

    lx_cache: Option<LexerState>,
    wd_cache: Option<String>,
}


impl Parser {
    pub fn new() -> Self {
        Parser {
            job: Job::new("".to_string()),
            cproc: Box::new(BuiltinProc(BuiltinProcess::default())),
            rd_buf: None,
            lx_cache: None,
            wd_cache: None,
        }
    }

    pub fn reset(&mut self) -> &mut Self {
        self.job = Job::new("".to_string());
        self.cproc = Box::new(BuiltinProc(BuiltinProcess::default()));
        self.rd_buf = None;
        self.lx_cache = None;
        self.wd_cache = None;

        self
    }

    fn pop_proc(&mut self) -> Box<ProcStruct> {
        mem::replace(&mut self.cproc,
                     Box::new(BuiltinProc(BuiltinProcess::default())))
    }

    pub fn eval(&mut self, sh: &mut Shell, cmd: String) -> (Option<Job>, LineState) {
        let cmd = cmd.trim().to_string();

        // FIXME: I believe this is no longer correct
        if cmd == "###" {
            return (None, LineState::Comment);
        }

        let lexer = if let Some(lx) = mem::replace(&mut self.lx_cache, None) {
            Lexer::with_state(cmd, lx)
        } else {
            Lexer::new(cmd)
        };

        for token_res in lexer {
            match token_res {
                // success cases
                Ok(TokenType::Word(tok)) => {
                    let tok = if let Some(mut xx) = mem::replace(&mut self.wd_cache, None) {
                        xx.push_str(&tok);
                        xx
                    } else {
                        tok
                    };

                    let tokv = tok_parse(sh, &tok);
                    for tok in tokv {
                        // gotta finish the redirect!
                        if self.rd_buf.is_some() {
                            let rdb = mem::replace(&mut self.rd_buf, None).unwrap();
                            self.cproc.push_arg(Arg::Rd(match rdb {
                                RedirBuf::RdArgOut => Redir::RdArgOut(tok),
                                RedirBuf::RdArgIn => Redir::RdArgIn(tok),
                                RedirBuf::RdFileOut(fd, ap) => Redir::RdFileOut(fd, tok, ap),
                                RedirBuf::RdFileIn(fd) => Redir::RdFileIn(fd, tok),
                                RedirBuf::RdStringIn(fd) => Redir::RdStringIn(fd, tok),
                            }));
                            self.rd_buf = None;
                            continue;
                        }

                        if !self.cproc.has_args() {
                            self.cproc = Box::new(match sh.st.resolve_exec(&tok) {
                                Some(sym::SymE::Builtin(b)) => BuiltinProc(BuiltinProcess::new(b)),
                                Some(sym::SymE::Binary(b)) => BinProc(BinProcess::new(&tok, b)),
                                Some(sym::SymE::Fn(f)) => BuiltinProc(BuiltinProcess::from_fn(f)),
                                None => {
                                    // TODO: 'command not found' here, yo
                                    if !sh.st.subsh {
                                        warn!("Command '{}' not found.", tok);
                                    }
                                    return (None, LineState::Normal);
                                }
                            });
                        } else {
                            self.cproc.push_arg(Arg::Str(tok));
                        }
                    }
                }
                Ok(TokenType::Pipe) => {
                    if self.rd_buf.is_some() {
                        warn!("Syntax error: unfinished redirect.");
                        self.rd_buf = None;
                    }
                    let p = self.pop_proc();
                    self.job.procs.push(p);
                }
                Ok(TokenType::Redir(rd_tok)) => {
                    let rd_tok = if let Some(mut xx) = mem::replace(&mut self.wd_cache, None) {
                        xx.push_str(&rd_tok);
                        xx
                    } else {
                        rd_tok
                    };

                    if self.rd_buf.is_some() {
                        warn!("Syntax error: unfinished redirect.");
                    }
                    let (rd, rdb) = redir_parse(rd_tok);
                    self.rd_buf = rdb;
                    if let Some(rd) = rd {
                        self.cproc.push_arg(Arg::Rd(rd));
                    }
                }
                Ok(TokenType::Block(tok)) => {
                    let tok = if let Some(mut xx) = mem::replace(&mut self.wd_cache, None) {
                        xx.push_str(&tok);
                        xx
                    } else {
                        tok
                    };

                    if self.rd_buf.is_some() {
                        warn!("Syntax error: unfinished redirect.");
                        self.rd_buf = None;
                    }
                    let tokv = tok.split('\n')
                        .filter_map(|l| {
                            if l.trim().is_empty() {
                                None
                            } else {
                                Some(l.trim().to_string())
                            }
                        })
                        .collect();
                    self.cproc.push_arg(Arg::Bl(tokv));
                }

                // Error cases
                Err(TokenException::ExtraRightParen) => {
                    warn!("Syntax error: extra right parenthesis found");
                    return (None, LineState::Normal);
                }
                Err(TokenException::ExtraRightBrace) => {
                    warn!("Syntax error: extra right brace found");
                    return (None, LineState::Normal);
                }
                Err(TokenException::Incomplete(lx_st, buf)) => {
                    self.lx_cache = Some(lx_st);
                    if self.wd_cache.is_some() {
                        let mut c = mem::replace(&mut self.wd_cache, None).unwrap();
                        c.push_str(&buf);
                        c.push_str("\n");
                        self.wd_cache = Some(c);
                    } else {
                        self.wd_cache = Some(buf);
                    }
                    return (None, LineState::Continue);
                }
            }
        }

        if self.rd_buf.is_some() {
            warn!("Syntax error: unfinished redirect.");
        }

        let p = self.pop_proc();
        self.job.procs.push(p);
        let new_job = mem::replace(&mut self.job, Job::new("".to_string()));
        (Some(new_job), LineState::Normal)
    }
}
