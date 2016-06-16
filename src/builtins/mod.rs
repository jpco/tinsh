mod set;

use std::collections::HashMap;
use std::process::exit;
use std::rc;
use std::fs;
use std::env;

use std::io;
use std::io::BufReader;
use std::io::BufRead;

use sym;

use sym::ScopeSpec;
use exec::Arg;
use shell::Shell;

#[derive(Clone)]
pub struct Builtin {
    pub name: &'static str,
    pub desc: &'static str,
    pub rd_cap: bool,
    pub bl_cap: bool,
    pub pat_cap: bool,
    pub run:  rc::Rc<Fn(Vec<Arg>, &mut Shell, Option<BufReader<fs::File>>) -> i32>
}

// TODO:
//  - elementary flow control
//  - hash
//  - __blank
//  - __fn_exec

fn blank_builtin() -> Builtin {
    Builtin {
        name: "__blank",
        desc: "The blank builtin",
        rd_cap: false, // is this true?
        bl_cap: true,
        pat_cap: false, // undefined but false
        run: rc::Rc::new(|args: Vec<Arg>, sh: &mut Shell,
                            _in: Option<BufReader<fs::File>>| -> i32 {
            let mut c = 0;
            // do nothing
            for a in args {
                c = match a {
                    Arg::Bl(bv) => {
                        sh.block_exec(bv)
                    }
                    _ => { c } // TODO: how to properly deal with this?
                };
            }

            c
        })
    }
}

pub fn fn_builtin(f: sym::Fn) -> Builtin {
    Builtin {
        name: "__fn_exec",
        desc: "The function execution builtin",
        rd_cap: false,
        bl_cap: true,
        pat_cap: true,
        run: rc::Rc::new(move |mut args: Vec<Arg>, sh: &mut Shell,
                               _in: Option<BufReader<fs::File>>| -> i32 {

            let lines = f.lines.clone();
            if !f.inline { sh.st.new_scope(true); }

            // parse/figure out the args
            // TODO: [] params
            for an in &f.args {
                let mut an = an as &str;
                let mut opt = false;
                if an.ends_with("?") {
                    opt = true;
                    an = &an[..an.len()-1];
                }
                if args.len() > 0 {
                    let _ = sh.st.set_scope(&an, args.remove(0).into_string(),
                                            ScopeSpec::Local);
                } else {
                    if !opt {
                        warn!("fn '{}': Not enough args provided", f.name);
                        return 2;  // TODO: care about this more
                    }
                }
            }

            if let &Some(ref va) = &f.vararg {
                let mut va = va as &str;
                if let &Some(ref pa) = &f.postargs {
                    for a in pa.iter().rev() {
                        let mut a = a as &str;
                        let mut opt = false;
                        if a.ends_with("?") {
                            opt = true;
                            a = &a[..a.len()-1];
                        }
                        match args.pop() {
                            Some(s) => {
                                let _ = sh.st.set_scope(a, s.clone().into_string(),
                                                        ScopeSpec::Local);
                            },
                            None => {
                                if !opt {
                                    warn!("fn '{}': Not enough args provided", f.name);
                                    return 2;
                                }
                            }
                        }
                    }
                }

                let mut opt = false;
                if va.ends_with("?") {
                    opt = true;
                    va = &va[..va.len()-1];
                }
                if args.len() > 0 {
                    let s = args.drain(..).map(|x| x.into_string()).collect::<Vec<String>>().join(" ");
                    let _ = sh.st.set_scope(&va, s, ScopeSpec::Local);
                } else {
                    if !opt {
                        warn!("fn '{}': Not enough args provided", f.name);
                        return 2;
                    }
                }
            }
            

            sh.input_loop(Some(lines), false);

            if !f.inline { sh.st.del_scope(); }

            sh.status_code
        })
    }
}

impl Default for Builtin {
    fn default() -> Self {
        blank_builtin()
    }
}

impl Builtin {
    pub fn map() -> HashMap<&'static str, Self> {
        let mut bi_map = HashMap::new();

        bi_map.insert(
            "ifx",
            Builtin {
                name: "ifx",
                desc: "Execute a block depending on the status code of a given statement",
                rd_cap: true,
                bl_cap: true,
                pat_cap: true,
                run: rc::Rc::new(|mut args: Vec<Arg>, sh: &mut Shell,
                                 _in: Option<BufReader<fs::File>>| -> i32 {
                    if args.len() == 0 {
                        warn!("Invalid number of arguments");
                        return 3;
                    }

                    if let Arg::Bl(lv) = args.pop().unwrap() {
                        let sc = sh.line_exec(args);

                        if sc == 0 {
                            sh.block_exec(lv)
                        } else {
                            sc
                        }
                    } else {
                        warn!("Must provide an executable block");
                        3
                    }
                })
            });

        bi_map.insert(
            "eval",
            Builtin {
                name: "eval",
                desc: "Evaluate a passed-in statement",
                rd_cap: false,
                bl_cap: true,
                pat_cap: true,
                run: rc::Rc::new(|args: Vec<Arg>, sh: &mut Shell,
                                    _in: Option<BufReader<fs::File>>| -> i32 {
                    sh.line_exec(args)
                })
            });

        bi_map.insert(
            "set",
            Builtin {
                name: "set",
                desc: "Set a variable binding",
                rd_cap: true,
                bl_cap: true,
                pat_cap: true,
                run: set::set_main()  // set is big, yeah yeah yeah
            });

        bi_map.insert(
            "cd",
            Builtin {
                name: "cd",
                desc: "Change directory",
                rd_cap: false,
                bl_cap: false,
                pat_cap: false,
                run: rc::Rc::new(|args: Vec<Arg>, _sh: &mut Shell,
                                 _in: Option<BufReader<fs::File>>| -> i32 {
                    // TODO: more smartly handle the case HOME is nothing?
                    if args.len() == 0 {
                        let home = match env::var("HOME") {
                            Ok(hm)  => hm,
                            Err(_)  => {
                                warn!("cd: no HOME environment variable found.");
                                return 2; /* TODO: correct error code */
                            }
                        };
                        match env::set_current_dir (home.clone()) {
                            Ok(_) => env::set_var("PWD", home),
                            Err(e) => {
                                warn!("cd: {}", e);
                                return 2;
                            }
                        };
                    } else {
                        let args: Vec<String> = args.into_iter()
                                                .filter_map(
                                                |x| if let Arg::Str(y) = x {
                                                    Some(y)
                                                } else { None }).collect();
                        let dest = match fs::canonicalize(args[0].clone()) {
                            Ok(pt) => pt,
                            Err(e) => {
                                warn!("cd: {}", e);
                                return 2;
                            }
                        }.into_os_string().into_string().unwrap();
                        match env::set_current_dir (dest.clone()) {
                            Ok(_) => env::set_var("PWD", dest),
                            Err(e) => {
                                warn!("cd: {}", e);
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
                name: "exit",
                desc: "Exit the tin shell",
                rd_cap: false,
                bl_cap: false,
                pat_cap: false,
                run: rc::Rc::new(|args: Vec<Arg>, _sh: &mut Shell,
                                 _in: Option<BufReader<fs::File>>| -> i32 {
                    if args.len() == 0 {
                        exit(0);
                    }
                   
                    // will always be true
                    if let Arg::Str(ref a) = args[0] {
                        match a.parse::<i32>() {
                            Ok(i) => exit(i),
                            Err(_) => {
                                warn!("exit: numeric argument required.");
                                exit(2)
                            }
                        }
                    } else { unreachable!(); }
                })
            });

        bi_map.insert(
            "history",
            Builtin {
                name: "history",
                desc: "List/control history",
                rd_cap: false,
                bl_cap: false,
                pat_cap: false,
                run: rc::Rc::new(|_args: Vec<Arg>, sh: &mut Shell,
                                 _in: Option<BufReader<fs::File>>| -> i32 {
                    sh.ht.hist_print();
                    0
                })
            });

        // TODO: this shouldn't even... be a real builtin? maybe?
        bi_map.insert(
            "read",
            Builtin {
                name: "read",
                desc: "Read from stdin or a file and echo to stdout",
                rd_cap: false,
                bl_cap: false,
                pat_cap: false,
                run: rc::Rc::new(|_args: Vec<Arg>, _sh: &mut Shell,
                                 inp: Option<BufReader<fs::File>>| -> i32 {
                    let mut in_buf = String::new();
                    let res = if let Some(mut br) = inp {
                        br.read_line(&mut in_buf)
                    } else {
                        io::stdin().read_line(&mut in_buf)
                    };
                    if res.is_ok() {
                        print!("{}", in_buf);
                        0
                    } else {
                        2
                    }
                })
            });

        bi_map
    }
}
