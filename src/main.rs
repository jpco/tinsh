mod exec;
mod prompt;
mod sym;
mod builtins;
mod eval;
mod shell;
mod hist;
mod err;
mod compl;

use std::env;

use prompt::LineState;
use prompt::Prompt;

use err::warn;
use err::info;
use err::err;

fn setup(opts: TinOpts) -> shell::Shell {
    let exec_rc;

    let rc_p: Box<Prompt> = match prompt::FilePrompt::new(&opts.config) {
        Ok(p) => {
            exec_rc = true;
            Box::new(p)
        },
        Err(e) => {
            exec_rc = false;
            err::info(&format!("Error loading .tinrc file: {}; skipping", e));
            Box::new(prompt::BasicPrompt)
        }
    };

    let mut sh = shell::Shell {
        pr: rc_p,
        ls: LineState::Normal,
        st: sym::Symtable::new(),
        ht: hist::Histvec::new()
    };

    // load the TinOpts struct into the symtable
    // TODO @me: learn to borrowck, scrub
    load_opts(opts.clone(), &mut sh.st);

    // exec .tinrc
    if exec_rc {
        main_loop(&mut sh);
    }

    // TODO: make the TinOpts struct vars readonly (here?)

    // if file/exec, do that
    if opts.file {
        let p: prompt::FilePrompt = prompt::FilePrompt::new(&opts.exec.unwrap())
                                    .unwrap();
        sh.pr = Box::new(p);
        main_loop(&mut sh);
    } else if let Some(cmd) = opts.exec {
        let p = prompt::CmdPrompt::new(cmd);
        sh.pr = Box::new(p);
        main_loop(&mut sh);
    }

    // if not interactive, exit
    // TODO: exit with last command's exit status
    if !opts.inter {
        std::process::exit(0);
    }

    sh.pr = Box::new(prompt::StdPrompt::new());

    sh
}

// This struct only exists at the beginning of the tinsh process;
// once the shell is running the options are accessible in the
// symtable (most of them won't do anything if changed after startup)
#[derive(Clone)]
struct TinOpts {
    inter: bool,  // __tin_inter (ro)
    login: bool,  // __tin_login (ro)
    noexec: bool, // __tin_noexec (ro)
    debug: u8,    // __tin_debug (rw)
    config: String, // __tinrc (ro) (idea: rw & enable config flush?)
    file: bool,
    exec: Option<String> // either __tin_filename or __tin_command (ro)
}

impl Default for TinOpts {
    fn default() -> Self {
        TinOpts {
            inter: true,
            login: false,
            noexec: false,
            debug: 1,
            config: format!("{}/.tinrc", env::var("HOME").unwrap_or("".to_string())),
            file: false,
            exec: None
        }
    }
}

fn load_opts(opts: TinOpts, st: &mut sym::Symtable) {
    if opts.inter {
        st.set("__tin_inter", "T".to_string());
    };

    // TODO: this may need to be readonly from the beginning
    if opts.login {
        st.set("__tin_login", "T".to_string());
    };

    if opts.noexec {
        st.set("__tin_noexec", "T".to_string());
    };

    st.set("__tin_debug", opts.debug.to_string());
    st.set("__tinrc", opts.config);

    if let Some(f) = opts.exec {
        match opts.file {
            true => { st.set("__tin_filename", f); },
            false => { st.set("__tin_command", f); }
        };
    }
}

fn read_args() -> TinOpts {
    let mut opts = TinOpts::default();

    // to be determined by -i arg and if exec is given
    opts.inter = false;

    let mut config_arg = false;
    let mut debug_arg = false;
    let mut command_arg = false;
    for arg in env::args().skip(1) {
        if config_arg {
            // last arg contained '-c'
            opts.config = arg;
            config_arg = false;
        } else if debug_arg {
            // last arg contained '-d'
            opts.debug = match &arg as &str {
                "debug" => 0,
                "warn"  => 1,
                "err"   => 2,
                _ => match arg.parse::<u8>() {
                    Ok(i)  => i,
                    Err(_) => {
                        warn(&format!("args: invalid debug level '{}' given", arg));
                        std::process::exit(2);
                    }
                }
            };
            debug_arg = false;
        } else if command_arg {
            // command to execute
            if opts.exec.is_some() {
                warn("args: multiple files/commands given to run");
            }
            opts.exec = Some(arg);
            command_arg = false;
        } else if arg.starts_with('-') {
            // actual arg letter parsing
            // TODO: why isn't real grapheme parsing a thing, rip
            for ch in arg.chars().skip(0) {
                match ch {
                    'i' => {
                        opts.inter = true;
                    },
                    'l' => {
                        opts.login = true;
                    },
                    'n' => {
                        opts.noexec = true;
                    },
                    'd' => {
                        debug_arg = true;
                    },
                    'e' => {
                        command_arg = true;
                    },
                    'c' => {
                        config_arg = true;
                    },
                    _ => { }
                }
                if debug_arg && (command_arg || config_arg) ||
                        command_arg && config_arg {
                    err("args: ambiguous arguments given");
                    std::process::exit(2);
                }
            }
        } else {
            if opts.exec.is_some() {
                warn("args: multiple files/commands given to run");
            } else {
                opts.file = true;
                opts.exec = Some(arg);
            }
        }
    }

    if opts.exec.is_none() {
        opts.inter = true;
    }

    opts
}

fn main_loop(mut sh: &mut shell::Shell) {
    // saves previous inputs in the case of LineState::Continue
    let mut input_buf = String::new();
    // saves "future" inputs in the case of multi-line input
    let mut next_buf: Option<String> = None;
    loop {
        // get input
        // "input" is the new input we are adding in this loop
        let input;

        if let Some(new_buf) = next_buf {
            // we have new things to deal with without prompting for more
            let (spl_input, spl_next_buf) = eval::spl_line(&new_buf);
            input = spl_input;
            next_buf = spl_next_buf;
        } else if let Some(prompt_in) = sh.pr.prompt(&sh.ls, &sh.st, &mut sh.ht) {
            // we needed more and we got more
            let (spl_input, spl_next_buf) = eval::spl_line(&prompt_in);
            input = spl_input;
            next_buf = spl_next_buf;
        } else {
            // nothing's left
            break;
        }

        // do stuff with input
        sh.ls = match sh.ls {
            LineState::Comment => {
                if input.trim() == "###" {
                    input_buf = String::new();
                    LineState::Normal
                } else {
                    LineState::Comment
                }
            },
            LineState::Normal | LineState::Continue => {
                input_buf.push_str(&input);

                let (t_job, ls) = eval::eval(&mut sh, input_buf.clone());
                
                if ls == LineState::Normal {
                    if let Some(t_job) = t_job {
                        sh.ht.hist_add (input_buf.trim());
                        t_job.exec(sh, true);
                    }
                    input_buf = String::new();
                }

                ls
            },
        };
    }

    // TODO: clean up all state! tear down all non-global scopes, set up
    // appropriate warnings, etc.
    if sh.ls != LineState::Normal {
        // warn or info?
        info("Line state left non-normal");
        sh.ls = LineState::Normal;
    }
}

fn main() {
    let mut sh;

    // startup
    {
        let opts = read_args();
        sh = setup(opts);
    }

    // interactive
    main_loop(&mut sh);
}
