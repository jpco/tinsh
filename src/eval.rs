use sym;
use exec;
use shell;
use err;

use std::process::Command;
extern crate unicode_segmentation;
use self::unicode_segmentation::UnicodeSegmentation;

use prompt::LineState;
use err::debug;
use err::warn;

pub fn eval (sh: &mut shell::Shell, cmd: String) -> (LineState, Vec<String>) {
    let cmd = cmd.trim().to_string();

    let mut new_ls = LineState::Normal;
    if cmd == "###" {
        new_ls = LineState::Comment;
    }

    let mut cmdword = String::new();
    let mut cmdvec = Vec::new();

    // paren status -- paren group can be evaluated once
    // count returns to zero.
    let mut parens: u8 = 0;
    // paren_idx = beginning of current paren group
    let mut paren_idx = 0;
    // quote status
    let mut quotes = ' ';
    // backslash
    let mut bs = false;

    for (i, c) in cmd.grapheme_indices(true) {
        if bs {
            cmdword.push_str(c);
            bs = false;
            continue;
        }

        if c.trim().is_empty() {
            if parens == 0 && quotes == ' ' && !cmdword.is_empty() {
                cmdvec.push(cmdword);
                cmdword = String::new();
            } else if quotes != ' ' {
                cmdword.push_str(c);
            }
            continue;
        }

        match c {
            "\"" => {
                if quotes == ' ' {
                    quotes = '"';
                } else if quotes == '"' {
                    quotes = ' ';
                }
            },
            "'"  => {
                if quotes == ' ' {
                    quotes = '\'';
                } else if quotes == '\'' {
                    quotes = ' ';
                }
            },
            "("  => {
                if quotes != '\'' {
                    if parens == 0 { // start of paren group!
                        paren_idx = i + 1;
                    }
                    parens = parens + 1;
                }
            }
            ")"  => {
                if quotes != '\'' {
                    if parens == 0 {
                        err::warn("Extra ')' found.");
                        continue;
                    }
                    parens = parens - 1;
                    if parens == 0 { // end of paren group!
                        if paren_idx < i {
                            match sh.st.resolve(&cmd[paren_idx..i]) {
                                Some(sym::Sym::Var(v)) => {
                                    cmdword.push_str(&v);
                                },
                                Some(sym::Sym::Binary(b)) => {
                                    // TODO: no unwrap()!
                                    cmdword.push_str(&format!("<binary '{}'>",
                                                     b.to_str().unwrap()));
                                },
                                Some(sym::Sym::Builtin(b)) => {
                                    cmdword.push_str(&format!("<builtin '{}'>", 
                                                     b.name));
                                },
                                None                       => {
                                    err::debug(&format!("Could not resolve '({})'.",
                                              &cmd[paren_idx..i]));
                                }
                            }
                        }
                    }
                }
            }
            "\\" => {
                bs = true;
            }
            _ => {
                if parens == 0 {
                    cmdword.push_str(c);
                }
            }
        }
    }

    if parens > 0 {
        err::warn("Unmatched parenthesis in line");
    }

    if quotes != ' ' {
        err::warn("Unmatched quote in line");
    }

    cmdvec.push(cmdword);

    (new_ls, cmdvec)
}

pub fn exec (mut cmdvec: Vec<String>, sh: &mut shell::Shell) {
    /*
    let mut cmdvec: Vec<String> = cmd.split_whitespace()
                                 .map(|x| x.to_string())
                                 .collect();
                                 */

    if cmdvec.len() == 0 {
        return;
    }

    let cmdname = cmdvec.remove(0);
    match sh.st.resolve(&cmdname) {
        Some(sym::Sym::Binary(cmdpath)) => {
            let mut cmd = Command::new(cmdpath);

            for arg in cmdvec {
                cmd.arg(arg);
            }

            exec::exec(cmd);
        },
        Some(sym::Sym::Builtin(bi_cmd)) => {
            (*bi_cmd.run)(cmdvec, sh);
        },
        Some(sym::Sym::Var(var_val)) => {
            debug(&format!("{}", var_val));
        },
        None => {
            // TODO: status code of 127.
            warn(&format!("Command '{}' could not be found.", cmdname));
        }
    }
}

