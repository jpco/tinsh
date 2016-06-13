#[macro_use] mod err;
mod prompt;
mod sym;
mod builtins;
mod parser;
mod lexer;
mod shell;
mod hist;
mod compl;
mod exec;
mod posix;
mod opts;

#[macro_use] extern crate lazy_static;

use std::env;

use prompt::LineState;

use shell::Shell;

fn setup(file: bool, exec: Option<String>) -> Shell {
    let mut sh = Shell {
        jobs: Vec::new(),
        status_code: 0,
        pr: Box::new(prompt::BasicPrompt),
        ls: LineState::Normal,
        st: sym::Symtable::new(),
        ht: hist::Histvec::new()
    };

    // interactive init (read rc file, posix::init)
    if opts::is_set("__tin_inter") {
        match prompt::FilePrompt::new(&opts::get("__tinrc").unwrap()) {
            Ok(p) => {
                sh.pr = Box::new(p);
                sh.input_loop(None, true);
                posix::init();
            },
            Err(e) => {
                info!("Error loading .tinrc file: {}; skipping", e);
            }
        };
    }

    // TODO: make the TinOpts struct vars readonly (here?)

    // if file/exec, do that
    if file {
        let p: prompt::FilePrompt = prompt::FilePrompt::new(&exec.unwrap())
                                    .unwrap();
        sh.pr = Box::new(p);
        sh.input_loop(None, true);
    } else if let Some(cmd) = exec {
        sh.input_loop(Some(vec![cmd]), false);
    }

    if opts::is_set("__tin_inter") {
        sh.pr = Box::new(prompt::StdPrompt::new());
    }

    sh
}

fn read_args() -> (bool, Option<String>) {
    let mut config_arg = false;
    let mut debug_arg = false;
    let mut command_arg = false;

    let mut inter = false;
    let mut config = None;
    let mut login = false;
    let mut noexec = false;
    let mut debug = None;
    let mut file = false;
    let mut exec = None;

    for arg in env::args().skip(1) {
        if config_arg {
            // last arg contained '-c'
            config = Some(arg);
            config_arg = false;
        } else if debug_arg {
            // last arg contained '-d'
            debug = Some(arg);
            debug_arg = false;
        } else if command_arg {
            // command to execute
            if exec.is_some() {
                warn!("args: multiple files/commands given to run");
            }
            exec = Some(arg);
            command_arg = false;
        } else if arg.starts_with('-') {
            // actual arg letter parsing
            // TODO: why isn't real grapheme parsing a thing, rip
            for ch in arg.chars().skip(0) {
                match ch {
                    'i' => {
                        inter = true;
                    },
                    'l' => {
                        login = true;
                    },
                    'n' => {
                        noexec = true;
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
            if exec.is_some() {
                warn!("args: multiple files/commands given to run");
            } else {
                file = true;
                exec = Some(arg);
            }
        }
    }

    let config = config.unwrap_or({
        let mut c = env::var("HOME").unwrap();
        c.push_str("/.inter.tin");
        c
    });

    let inter = inter || (posix::is_interactive() && exec.is_none());

    // initialize table, set ro opts
    opts::init(inter, login, noexec, config);

    // set rw opts
    if let Some(debug) = debug {
        if let Err(_) = opts::set("__tin_debug", debug) {
            err!("args: Invalid debug option set at startup");
            std::process::exit(2);
        }
    }

    (file, exec)
}

fn main() {
    let mut sh;

    let (file, exec) = read_args();
    let do_inter = exec.is_none();
    sh = setup(file, exec);

    // interactive
    if do_inter {
        sh.input_loop(None, true);
        if opts::is_set("__tin_inter") {
            println!("exit");
        }
    }
}
