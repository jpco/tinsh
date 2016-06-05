use std::io;
use std::io::Write;

static mut dbg_thresh: u8 = 1;

pub fn debug_setthresh (new: u8) {
    unsafe {
        dbg_thresh = new;
    }
}

pub fn debug_print(msg: &str, level: u8) {
    let thresh;
    unsafe {
        thresh = dbg_thresh;
    }
    if level >= thresh {
        let _ = writeln!(&mut io::stderr(), "{}: {}", match level {
            3 => "Error",
            2 => "Warning",
            1 => "Info",
            _ => "Debug"
        }, msg);
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
