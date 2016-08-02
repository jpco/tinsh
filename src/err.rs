use std::io;
use std::io::Write;
use opts;

pub fn debug_print(msg: &str, level: u8) {
    if level >= opts::get("__tin_debug").unwrap().parse::<u8>().unwrap() {
        let _ = writeln!(&mut io::stderr(),
                         "{}: {}",
                         match level {
                             3 => "Error",
                             2 => "Warning",
                             1 => "Info",
                             _ => "Debug",
                         },
                         msg);
    }

    if level == 3 {
        panic!("Unrecoverable error encountered");
    }
}

macro_rules! debug {
    ($($arg:tt)* ) => ($crate::err::debug_print(&format!($($arg)*), 0))
}

macro_rules! info {
    ( $($arg:tt)* ) => ($crate::err::debug_print(&format!($($arg)*), 1))
}

macro_rules! warn {
    ( $($arg:tt)* ) => ($crate::err::debug_print(&format!($($arg)*), 2))
}

macro_rules! err {
    ( $($arg:tt)* ) => ($crate::err::debug_print(&format!($($arg)*), 3))
}
