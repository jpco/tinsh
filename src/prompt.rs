use std::io::prelude::*;
use std::io;

pub enum LineState {
    Normal,
    Comment
}

pub trait Prompt {
    fn prompt(&self, ls: &LineState) -> String;
}

pub struct BasicPrompt;

impl Prompt for BasicPrompt {   
    fn prompt(&self, ls: &LineState) -> String {
        print!("$ ");
        io::stdout().flush().ok().expect("Could not flush stdout");

        let mut buf = String::new();
        io::stdin().read_line(&mut buf)
            .expect("Could not read stdin");

        buf
    }
}
