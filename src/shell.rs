use prompt::Prompt;
use prompt::LineState;
use sym::Symtable;
use hist::Histvec;

/// terrible God object to make state accessible to everyone everywhere
pub struct Shell {
    pub pr: Box<Prompt>,
    pub ls: LineState,
    pub st: Symtable,
    pub ht: Histvec
}
