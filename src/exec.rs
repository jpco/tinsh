use std::process::Command;

//////////
// TODO: make this correct
// (POSIX compliant, able to bg/fg jobs, give it a notion of 'job',
// lots to do here)
//////////
pub fn exec (mut cmd: Command) {
    match cmd.spawn() {
        Ok(mut ch) => match ch.wait() {
            Ok(_) => { },
            Err(e) => println!("Error waiting for child: {}", e)
        },
        Err(e) => println!("Error: {}", e)
    }
}
