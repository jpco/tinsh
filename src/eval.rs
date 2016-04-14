use sym;
use exec;
use shell;

use prompt::LineState;
use std::process::Command;

use err::debug;
use err::warn;

pub fn eval (cmd: String) -> (LineState, String) {
    let cmd = cmd.trim().to_string();

    let mut new_ls = LineState::Normal;
    if cmd == "###" {
        new_ls = LineState::Comment;
    }

    (new_ls, cmd)
}

pub fn exec (cmd: String, sh: &mut shell::Shell) {
    let mut cmdvec: Vec<String> = cmd.split_whitespace()
                                     .map(|x| x.to_string())
                                     .collect();

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

