static mut dbg_thresh: u8 = 1;

pub fn debug_setthresh (new: u8) {
    unsafe {
        dbg_thresh = new;
    }
}

fn debug_print(msg: &str, level: u8) {
    let thresh;
    unsafe {
        thresh = dbg_thresh;
    }
    if level >= thresh {
        println!("{}: {}", match level {
            3 => "Error",
            2 => "Warning",
            1 => "Info",
            _ => "Debug"
        }, msg);
    }

    if level == 3 {
        // TODO: we can do this later
        /* let noerr = match st.resolve("__tin_noerr") {
            Some(sym::Sym::Var(x)) => x,
            _                      => "".to_string()
        };

        if noerr.len() == 0 { */
            panic!("Unrecoverable error encountered");
        // } else if thresh == 0 {
            // println!("Debug: Recovering from error");
        // }
    }
}

pub fn debug (msg: &str) {
    debug_print(msg, 0);
}

pub fn info (msg: &str) {
    debug_print(msg, 1);
}

pub fn warn (msg: &str) {
    debug_print(msg, 2);
}

pub fn err (msg: &str) {
    debug_print(msg, 3);
}
