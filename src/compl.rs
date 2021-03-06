use std::env;
use std::fs;

use sym::Symtable;

extern crate glob;

fn common_prefix(mut strs: Vec<String>) -> String {
    let mut c_prefix = String::new();
    if strs.len() == 0 {
        return c_prefix;
    }

    let delegate = strs.pop().unwrap().clone();
    for (di, dc) in delegate.chars().enumerate() {
        for ostr in &strs {
            match ostr.chars().nth(di) {
                Some(oc) => {
                    if dc != oc {
                        return c_prefix;
                    }
                }
                None => {
                    return c_prefix;
                }
            }
        }
        c_prefix.push(dc);
    }

    c_prefix
}

fn print_completions(res_vec: &Vec<String>) {
    // TODO: prettier
    println!("");
    for p in res_vec {
        println!("{}", p);
    }
}

pub fn complete(in_str: &str, st: &Symtable, first_word: bool, print_multi: bool) -> String {
    if first_word && !in_str.contains('/') {
        bin_complete(in_str, st, print_multi)
    } else {
        fs_complete(in_str, print_multi)
    }
}

pub fn bin_complete(in_str: &str, st: &Symtable, print_multi: bool) -> String {
    let res_vec = st.prefix_resolve_exec(in_str);

    if res_vec.len() == 1 {
        let mut res = res_vec[0].clone();
        res.push(' ');
        res
    } else if res_vec.len() == 0 {
        in_str.to_string()
    } else {
        if print_multi {
            print_completions(&res_vec);
        }
        common_prefix(res_vec)
    }
}

// TODO: more completely handle globs in the to-complete string
pub fn fs_complete(in_str: &str, print_multi: bool) -> String {
    let mut input;
    let mut tilde = false;
    let mut cpref = false;
    if in_str.starts_with("~/") {
        tilde = true;
        let in_str = in_str.trim_left_matches("~");
        input = env::var("HOME").unwrap_or("~/".to_string());
        input.push_str(in_str);
    } else {
        if in_str.starts_with("./") {
            cpref = true;
        }
        input = in_str.to_string();
    }

    if !input.ends_with('*') {
        input.push('*');
    }

    let mut postfix = "";
    let res_vec = match glob::glob(&input) {
            Ok(x) => x,
            Err(_) => {
                return in_str.to_string();
            }
        }
        .filter_map(Result::ok)
        .map(|x| format!("{}", x.display()))
        .collect::<Vec<String>>();

    let mut ret = if res_vec.len() == 1 {
        let ref res = res_vec[0];

        postfix = match fs::metadata(res) {
            Ok(x) => {
                if x.is_dir() {
                    "/"
                } else {
                    " "
                }
            }
            Err(_) => "",
        };

        res.clone()
    } else if res_vec.len() > 0 {
        if print_multi {
            print_completions(&res_vec);
        }

        // find common prefix
        common_prefix(res_vec)
    } else {
        in_str.to_string()
    };

    // post-facto formatting
    ret = ret.replace(" ", "\\ ");
    ret.push_str(postfix);

    if tilde {
        ret.replace(&env::var("HOME").unwrap(), "~")
    } else {
        if cpref && !ret.starts_with("./") {
            ret.insert(0, '.');
            ret.insert(1, '/');
        }
        ret
    }
}
