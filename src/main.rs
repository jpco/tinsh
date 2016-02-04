mod fork;
mod prompt;

use prompt::LineState;
use prompt::Prompt;

fn setup() -> Box<prompt::Prompt> {
    Box::new(prompt::BasicPrompt)
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
    let pr = setup();

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
            LineState::Normal => eval (&input)
        };
    }
}
