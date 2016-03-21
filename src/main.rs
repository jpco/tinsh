mod exec;
mod prompt;
mod sym;

use prompt::LineState;
use prompt::Prompt;
use std::process::{Command, exit};

fn setup() -> Box<Prompt> {
    Box::new(prompt::StdPrompt::init())
}

fn eval (cmd: &String, symt: &mut sym::Symtable) -> LineState {
    let cmd = cmd.trim();

    if cmd == "###" {
        return LineState::Comment;
    }

    let mut cmdvec: Vec<&str> = cmd.split(' ').collect();

    let cmdname = cmdvec.remove(0);
    match symt.resolve(cmdname) {
        Some(sym) => {
            let sym::Sym::Binary(cmdpath) = sym;
            let mut cmd = Command::new(cmdpath);

            for arg in cmdvec {
                cmd.arg(arg);
            }

            exec::exec(cmd);
        },
        None => {
            // TODO: status code of 127.
            println!("Command '{}' could not be found.", cmdname);
        }
    }

    LineState::Normal
}

fn main() {
    let mut pr = setup();

    let mut ls = LineState::Normal;

    let mut symt = sym::Symtable::new();
    symt.hash_bins();

    loop {
        let input = pr.prompt(&ls);
        ls = match ls {
            LineState::Comment => {
                if input.trim() == "###" {
                    LineState::Normal
                } else {
                    LineState::Comment
                }
            },
            LineState::Normal => {
                if input.trim() == "exit" {
                    exit(0);
                } else { 
                    eval (&input, &mut symt)
                }
            }
        };
    }
}
