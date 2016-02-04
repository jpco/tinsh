use std::io::prelude::*;
use std::io;
use std::ffi::CString;
use std::process::exit;

extern crate libc;

fn setup() -> (fn(&LineState) -> String) {
    basicprompt
}

fn basicprompt(ls: &LineState) -> String {
    print!("$ ");
    io::stdout().flush().ok().expect("Could not flush stdout");

    let mut buf = String::new();
    io::stdin().read_line(&mut buf)
        .expect("Could not read stdin");

    buf
}

enum LineState {
    Normal,
    Comment
}

fn eval (cmd: &String) -> LineState {
    let cmd = cmd.trim();

    if cmd == "###" {
        return LineState::Comment;
    }

    fork(cmd.split(' ').collect());

    LineState::Normal
}

// TODO: make this not suck as much
fn fork (argv: Vec<&str>) {
    // convert from Vec<&str> to what C wants
    let mut res: i32 = 0;
    let mut c_argv: Vec<*const libc::c_char> = Vec::new();
    for arg in &argv {
        let arg = arg.trim();
        let res = CString::new(arg as &str);
        match res {
            Ok(val) => c_argv.push(val.into_raw()),
            Err(_)  => panic!("fork (string conversion)")
        }
    }
    c_argv.push(std::ptr::null());

    let pid: libc::pid_t;
    unsafe {
        pid = libc::fork();
    }
    if pid < 0 {
        panic!("fork failed");
    } else if pid > 0 {
        unsafe {
            libc::waitpid(pid, &mut res, 0);
        }
        return;
    }

    unsafe {
        res = libc::execv(c_argv[0], c_argv.as_ptr());
    }

    if res == -1 {
        println!("No such file or directory.");
    } else {
        println!("Error executing '{}': {}", argv[0].trim(), res);
    }

    // shouldn't get here, but if we do get here we just want to get the heck out
    exit (127);
}

fn main() {
    let input_fn = setup();

    let mut ls = LineState::Normal;

    loop {
        let input = input_fn(&ls);
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
