mod exec;
mod prompt;
mod sym;
mod builtins;
mod eval;
mod shell;
mod hist;

use prompt::LineState;
use prompt::Prompt;

fn setup() -> Box<Prompt> {
    Box::new(prompt::StdPrompt::init())
    // Box::new(prompt::BasicPrompt)
}

fn main() {
    let mut sh = shell::Shell {
        pr: setup(),
        ls: LineState::Normal,
        st: sym::Symtable::new(),
        ht: hist::Histvec::new()
    };

    loop {
        let input = sh.pr.prompt(&sh.ls, &sh.st, &mut sh.ht);
        sh.ls = match sh.ls {
            LineState::Comment => {
                if input.trim() == "###" {
                    LineState::Normal
                } else {
                    LineState::Comment
                }
            },
            LineState::Normal => {
                sh.ht.hist_add (&input);
                let (ls, cmd_line) = eval::eval (input);
                match ls {
                    LineState::Normal => eval::exec (cmd_line, &mut sh),
                    _ => { }
                };

                ls
            }
        };
    }
}
