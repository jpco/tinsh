use std::collections::HashMap;
use std::process::exit;
use std::rc;
use std::fs;
use std::env;

use shell::Shell;
use err::warn;

#[derive(Clone)]
pub struct Builtin {
    pub desc: &'static str,
    pub run:  rc::Rc<Fn(Vec<String>, &mut Shell) -> i32>
}

impl Builtin {
    pub fn map() -> HashMap<&'static str, Self> {
        let mut bi_map = HashMap::new();

        bi_map.insert(
            "set",
            Builtin {
                desc: "Set a variable binding",
                run: rc::Rc::new(|args: Vec<String>, sh: &mut Shell| -> i32 {
                    if args.len() < 2 {
                        warn(&mut sh.st, "set: insufficient arguments.");
                        return 2;
                    }
                    sh.st.set(&args[0], args[1].clone());

                    0
                })
            });

        bi_map.insert(
            "cd",
            Builtin {
                desc: "Change directory",
                run: rc::Rc::new(|args: Vec<String>, sh: &mut Shell| -> i32 {
                    // TODO: more smartly handle the case HOME is nothing?
                    if args.len() == 0 {
                        let home = match env::var("HOME") {
                            Ok(hm)  => hm,
                            Err(_)  => {
                                warn(&mut sh.st, "cd: no HOME environment variable found.");
                                return 2; /* TODO: correct error code */
                            }
                        };
                        match env::set_current_dir (home.clone()) {
                            Ok(_) => env::set_var("PWD", home),
                            Err(e) => {
                                warn(&mut sh.st, &format!("cd: {}", e));
                                return 2;
                            }
                        };
                    } else {
                        let dest = match fs::canonicalize(args[0].clone()) {
                            Ok(pt) => pt,
                            Err(e) => {
                                warn(&mut sh.st, &format!("cd: {}", e));
                                return 2;
                            }
                        }.into_os_string().into_string().unwrap();
                        match env::set_current_dir (dest.clone()) {
                            Ok(_) => env::set_var("PWD", dest),
                            Err(e) => {
                                warn(&mut sh.st, &format!("cd: {}", e));
                                return 2;
                            }
                        };
                    }

                    0
                })
            });

        bi_map.insert(
            "exit",
            Builtin {
                desc: "Exit the tin shell",
                run: rc::Rc::new(|args: Vec<String>, sh: &mut Shell| -> i32 {
                    if args.len() == 0 {
                        exit(0);
                    } 
                    match args[0].parse::<i32>() {
                        Ok(i) => exit(i),
                        Err(_) => {
                            warn(&mut sh.st, "exit: numeric argument required.");
                            exit(2)
                        }
                    }
                })
            });

        bi_map.insert(
            "history",
            Builtin {
                desc: "List/control history",
                run: rc::Rc::new(|_args: Vec<String>, sh: &mut Shell| -> i32 {
                    sh.ht.hist_print();
                    0
                })
            });

        bi_map
    }
}
