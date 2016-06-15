use std::io::BufReader;
use std::fs;
use std::rc;

use sym;

use exec;
use exec::Arg;
use exec::Redir;
use shell::Shell;

fn rd_set(_rd: Redir) -> i32 {
    println!("Redirection set is unimplemented");
    0
}

fn set_spec(av: &mut Vec<Arg>) -> sym::ScopeSpec {
    let mut ret = sym::ScopeSpec::Default;

    while av.len() > 0 {
        if av[0].is_str() {
            if !av[0].as_str().starts_with("-") { break; }
            let s = av.remove(0).unwrap_str();

            // FIXME: graphemes()?
            for c in s.chars().skip(1) {
                ret = match c {
                    'l' => sym::ScopeSpec::Local,
                    'g' => sym::ScopeSpec::Global,
                    'e' => sym::ScopeSpec::Environment,
                    _ => {
                        warn!("set: Unrecognized argument '{}' found.", c);
                        ret
                    }
                }
            }
        } else { break; }
    }
    
    ret
}

fn set_keys(av: &mut Vec<Arg>) -> Vec<String> {
    let mut ret = Vec::new();

    while av.len() > 0 {
        let arg = av.remove(0);
        
        // check for '='
        if let Arg::Str(ref s) = arg {
            if s == "=" { break; }
        }

        for k in arg.into_vec() { ret.push(k); }
    }

    ret
}

fn fn_set(kv: Vec<String>, av: Vec<Arg>) -> i32 {
    println!("fn set is unimplemented.");
    0
}

pub fn set_main() -> 
    rc::Rc<Fn(Vec<Arg>, &mut Shell, Option<BufReader<fs::File>>) -> i32> {
    rc::Rc::new(|mut args: Vec<Arg>, sh: &mut Shell, _in: Option<BufReader<fs::File>>| -> i32 {
        // rd-set
        if args.len() == 1 {
            if args[0].is_rd() {
                let rd = args.remove(0).unwrap_rd();
                return rd_set(rd);
            }
        }

        // get args and keys
        let spec = set_spec(&mut args);
        let mut keyv = set_keys(&mut args);

        // filter out invalid keys
        let keyv = keyv.drain(..).filter(|a| a.find(|x| {
            if "?! {}()".contains(x) {  // TODO: more invalid chars
                warn!("set: Key '{}' contains invalid characters", a); true
            } else { false }
        }).is_none()).collect::<Vec<String>>();

        // if we just said 'set a b c', we want to set them to empty
        if args.is_empty() { args.push(Arg::Str(String::new())); }

        if args[0].is_str() && args[0].as_str() == "fn" {
            return fn_set(keyv, args);
        }

        let val = args.drain(..).flat_map(|x| x.into_vec()).collect::<Vec<String>>().join(" ");

        let mut r = 0;
        for k in keyv {
            if sh.st.set_scope(&k, val.clone(), spec).is_err() { r = 2; }
        }
        r
    })
}
