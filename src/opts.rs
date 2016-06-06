use std::collections::HashMap;
use std::sync::{Arc, Once, ONCE_INIT};
use std::error::Error;
use std::fmt;
use std::mem;
use std::string;

#[derive(Clone)]
struct Opt {
    val: Option<String>,
    wr_cond: Arc<Fn(Option<&string::String>) -> bool>
}

// EEP! Mutable globals are not pretty!
// They are also generally a bad idea. This module is partially to ensure
// safety while using this.
//
// TODO: use Mutexes to get better thread-safety on the Opts.
fn table() -> &'static mut HashMap<&'static str, Opt> {
    static mut OPT_TABLE: *mut HashMap<&'static str, Opt> = 
                        0 as *mut HashMap<&'static str, Opt>;
    static ONCE: Once = ONCE_INIT;

    // initialize table
    unsafe {
        ONCE.call_once(|| {
            let ot: HashMap<&'static str, Opt> = HashMap::new();

            OPT_TABLE = mem::transmute(Box::new(ot));
        });

        &mut (*OPT_TABLE)
    }
}

pub fn init(inter: bool, login: bool, noexec: bool, 
            tinrc: String) {
    // fill table
    macro_rules! bool2str {
        ($x:expr) => (if $x {
            Some("y".to_string())
        } else {
            None
        });
    };
    macro_rules! ro_opt {
        () => (Opt {
            val: None,
            wr_cond: Arc::new(|_| false)
        });
        ($x:expr) => (Opt {
            val: $x,
            wr_cond: Arc::new(|_| false)
        });
    };
    macro_rules! rw_opt {
        () => (Opt {
            val: None,
            wr_cond: Arc::new(|_| true)
        });
        ($x:expr) => (Opt {
            val: $x,
            wr_cond: Arc::new(|_| true)
        });
    };

    let mut t = table();

    t.insert("__tin_inter", ro_opt!(bool2str!(inter)));
    t.insert("__tin_login", ro_opt!(bool2str!(login)));
    t.insert("__tin_noexec", ro_opt!(bool2str!(noexec)));
    t.insert("__tinrc", ro_opt!(Some(tinrc)));
    t.insert("__tin_debug", Opt {
        val: Some("1".to_string()),
        wr_cond: Arc::new(|x| {
            if let Some(x) = x {
                match x as &str {
                    "debug" | "info" | "warn" | "err" => true,
                    _ => match x.parse::<u8>() {
                        Ok(i) => i <= 3,
                        Err(_) => false
                    }
                }
            } else { false }
        })}); // oof

    // TODO: set opts based on interactivity
    // TODO: actually use these
    t.insert("__tin_safemode", rw_opt!());
    t.insert("__tin_eundef", rw_opt!());
    t.insert("__tin_ecode", rw_opt!());
    t.insert("__tin_ewarn", rw_opt!());
    t.insert("__tin_pipefail", rw_opt!());
    t.insert("__tin_globmode", rw_opt!());
    t.insert("__tin_globcompl", rw_opt!());
    t.insert("__tin_psplit", rw_opt!());
}

pub fn is_opt(key: &str) -> bool {
    table().contains_key(key)
}

pub fn is_set(key: &str) -> bool {
    match table().get(key) {
        Some(&Opt { val: Some(_), .. }) => true,
        Some(_) => false,
        None => {
            panic!("key: {}", key) // Do not do this.
        }
    }
}

pub fn get(key: &str) -> Option<String> {
    match table().get(key) {
        Some(&Opt {val: ref os, .. }) => os.clone(),
        None => panic!() // Don't do this.
    }
}

#[derive(Debug)]
pub struct OptError {
    msg: String
}

impl fmt::Display for OptError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{}", self.msg)
    }
}

impl Error for OptError {
    fn description(&self) -> &str {
        &self.msg
    }
    fn cause(&self) -> Option<&Error> {
        None // U know the cause
    }
}

pub fn set(key: &str, nv: String) -> Result<(), OptError> {
    _iset(key, Some(nv))
}

pub fn unset(key: &str) -> Result<(), OptError> {
    _iset(key, None)
}

fn _iset(key: &str, nv: Option<String>) -> Result<(), OptError> {
    match table().get_mut(key) {
        Some(o) => {
            if (*o.wr_cond)(nv.as_ref()) {
                o.val = nv;
                Ok(())
            } else {
                Err(OptError {
                    msg: format!("Could not write value to key '{}'", key)
                })
            }
        },
        None => panic!()
    }
}
