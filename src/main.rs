mod fork;
mod prompt;

use prompt::LineState;
use prompt::Prompt;
use std::process::exit;

fn setup() -> Box<Prompt> {
    Box::new(prompt::StdPrompt::init())
}

fn eval (cmd: &String) -> LineState {
    let cmd = cmd.trim();

    if cmd == "###" {
        return LineState::Comment;
    }

    fork::fork(cmd.split(' ').collect());

    LineState::Normal
}

fn main() {
    let mut pr = setup();

    let mut ls = LineState::Normal;

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
                    eval (&input)
                }
            }
        };
    }
}
