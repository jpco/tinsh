use std::process::Command;

//////////
// TODO: make this correct
// (POSIX compliant, able to bg/fg jobs, give it a notion of 'job',
// lots to do here)
//////////
pub fn exec (mut cmd: Command) {
    cmd.spawn().unwrap().wait();
}
