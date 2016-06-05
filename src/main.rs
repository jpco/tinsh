#[macro_use] mod err;
mod prompt;
mod sym;
mod builtins;
mod eval;
mod shell;
mod hist;
mod compl;
mod exec;
mod posix;

#[macro_use] extern crate lazy_static;

use std::env;

use prompt::LineState;

use shell::Shell;

fn setup(opts: TinOpts) -> Shell {
    let mut sh = Shell {
        jobs: Vec::new(),
        interactive: posix::is_interactive() || opts.inter,
        pr: Box::new(prompt::BasicPrompt),
        ls: LineState::Normal,
        st: sym::Symtable::new(),
        ht: hist::Histvec::new()
    };

    // load the TinOpts struct into the symtable
    load_opts(&opts, &mut sh.st);

    // interactive init (read rc file, posix::init)
    if sh.interactive {
        match prompt::FilePrompt::new(&opts.config) {
            Ok(p) => {
                sh.pr = Box::new(p);
                sh.input_loop(None);
                posix::init();
            },
            Err(e) => {
                info!("Error loading .tinrc file: {}; skipping", e);
            }
        };
    }

    // TODO: make the TinOpts struct vars readonly (here?)

    // if file/exec, do that
    if opts.file {
        let p: prompt::FilePrompt = prompt::FilePrompt::new(&opts.exec.unwrap())
                                    .unwrap();
        sh.pr = Box::new(p);
        sh.input_loop(None);
    } else if let Some(cmd) = opts.exec {
        let p = prompt::CmdPrompt::new(cmd);
        sh.pr = Box::new(p);
        sh.input_loop(None);
    }

    if sh.interactive {
        sh.pr = Box::new(prompt::StdPrompt::new());
    }

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
            inter: posix::is_interactive(),
            login: false,
            noexec: false,
            debug: 1,
            config: format!("{}/.inter.tin", env::var("HOME").unwrap_or("".to_string())),
            file: false,
            exec: None
        }
    }
}

fn load_opts(opts: &TinOpts, st: &mut sym::Symtable) {
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
    st.set("__tinrc", opts.config.to_string());

    if let Some(ref f) = opts.exec {
        match opts.file {
            true => { st.set("__tin_filename", f.to_string()); },
            false => { st.set("__tin_command", f.to_string()); }
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
                        warn!("args: invalid debug level '{}' given", arg);
                        std::process::exit(2);
                    }
                }
            };
            debug_arg = false;
        } else if command_arg {
            // command to execute
            if opts.exec.is_some() {
                warn!("args: multiple files/commands given to run");
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
                    err!("args: ambiguous arguments given");
                    std::process::exit(2);
                }
            }
        } else {
            if opts.exec.is_some() {
                warn!("args: multiple files/commands given to run");
            } else {
                opts.file = true;
                opts.exec = Some(arg);
            }
        }
    }

    opts
}

fn main() {
    let mut sh;

    // startup
    {
        let opts = read_args();
        sh = setup(opts);
    }

    // interactive
    sh.input_loop(None);
    if sh.interactive {
        println!("exit");
    }
}
