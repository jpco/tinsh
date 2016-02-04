extern crate libc;

use std::ffi::CString;
use std::process::exit;
use std::ptr;

//////////
// TODO: make this correct
// (POSIX compliant, able to bg/fg jobs, give it a notion of 'job',
// lots to do here)
//////////
pub fn fork (argv: Vec<&str>) {
    // convert from Vec<&str> to what C wants
    // is there a better way to do this?
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
    c_argv.push(ptr::null());

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

