#![allow(dead_code)]

use builtins;
use std::collections::HashMap;
use std::env;
use std::fs;
use std::path;

use err;

#[derive(PartialEq)]
pub enum SymType {
    Binary,
    Builtin,
    Var,
    Environment
}

pub enum Sym {
    Binary(path::PathBuf),
    Builtin(builtins::Builtin),
    Var(String),
    Environment(String)
}

struct VarVal {
    val: String,
    readonly: bool
}

// the 'is_fn' flag enables us to check
// whether the current scope is the top-level scope of
// a function -- aside from global ops, we don't want to act
// through the function barrier.
struct Scope {
    vars: HashMap<String, VarVal>,
    is_fn: bool,
    is_gl: bool
}

#[derive(PartialEq)]
pub enum ScopeSpec {
    Local,
    Global,
    Environment,
    Default
}


// We can use 'static for builtins because that's what builtins are: static.
pub struct Symtable {
    bins:     HashMap<String, path::PathBuf>,
    builtins: HashMap<&'static str, builtins::Builtin>,
    scopes:   Vec<Scope>
}

impl Symtable {
    pub fn new() -> Symtable {
        let mut st = Symtable {
            bins:     HashMap::new(),
            builtins: builtins::Builtin::map(),
            scopes:   Vec::new()
        };

        st.scopes.push(Scope {
            vars: HashMap::new(),
            is_fn: false,
            is_gl: true
        });

        st.hash_bins();

        st
    }
   
    fn set_debug(&mut self, val: String) {
        let i = match &val as &str {
            "debug" => 0,
            "info"  => 1,
            "warn"  => 2,
            "err"   => 3,
            _ => match val.parse::<u8>() {
                Ok(i)  => i,
                Err(_) => {
                    err::warn(&format!("args: invalid __tin_debug level '{}' given", val));
                    return;
                }
            }
        };

        err::debug_setthresh(i);
    }

    pub fn set(&mut self, key: &str, val: String) -> &mut Symtable {
        self.set_scope(key, val, ScopeSpec::Default)
    }

    // TODO: check readonly
    pub fn set_scope(&mut self, key: &str, val: String, sc: ScopeSpec)
            -> &mut Symtable {
        if key == "__tin_debug" {
            self.set_debug(val);
            return self;
        }

        if sc == ScopeSpec::Environment {
            env::set_var(key, val);
            return self;
        }

        { // need to scope this for borrowck
            let scope = match sc {
                ScopeSpec::Global => { &mut self.scopes[0].vars },
                ScopeSpec::Local => { &mut self.scopes.last_mut().unwrap().vars },
                ScopeSpec::Environment => { unreachable!() },
                ScopeSpec::Default => {
                    let mut ret = None;
                    let last_idx = self.scopes.len() - 1;
                    for (idx, scope) in self.scopes.iter_mut().rev().enumerate() {
                        if scope.vars.get(key).is_some() {
                            ret = Some(scope);
                            break;
                        }

                        if idx == last_idx {
                            ret = Some(scope);
                        }
                    }
                    &mut ret.unwrap().vars
                }
            };

            if val == "" {
                scope.remove(key);
            } else {
                scope.insert(key.to_string(), VarVal { val: val, readonly: false });
            }
        }

        self
    }

    pub fn set_readonly(&mut self, key: &str, val: String) -> &mut Symtable {
        self.scopes.last_mut().unwrap().vars.insert(key.to_string(), VarVal {
            val: val,
            readonly: true
        });

        self
    }

    pub fn new_scope(&mut self, is_fn: bool) -> &mut Symtable {
        self.scopes.push(Scope {
            vars: HashMap::new(),
            is_fn: is_fn,
            is_gl: false
        });

        self
    }

    pub fn del_scope(&mut self) -> &mut Symtable {
        // we must always have a scope
        // error handling re: a bogus '}' is elsewhere
        if self.scopes.len() > 1 {
            self.scopes.pop();
        }

        self
    }

    fn hash_bins(&mut self) -> &mut Symtable {
        self.bins.clear();

        let path_str = env::var("PATH").unwrap_or("/bin:/usr/bin".to_string());
        for path_dir in path_str.split(":") {
            if let Ok(path_dir) = fs::read_dir(path_dir) {
                for path_f in path_dir {
                    if let Err(e) = path_f {
                        println!("Error: {}", e);
                        break;
                    }
                    let path_f = path_f.unwrap();
                    let is_ok_f = match path_f.metadata() {
                        Ok(ent) => !ent.is_dir(),   // FIXME: should only be executable files
                        Err(_) => false
                    };

                    if is_ok_f {
                        if let Some(os_fname) = path_f.path().file_name() {
                            if let Ok(fname) = os_fname.to_os_string().into_string() {
                                self.bins.insert(fname, path_f.path());
                            }
                        }
                    }
                }
            }
        }

        self
    }

    pub fn prefix_resolve(&self, sym_n: &str) -> Vec<String> {
        self.prefix_resolve_types(sym_n, None)
    }

    // simply resolves a vector of matching strings.
    pub fn prefix_resolve_types(&self, sym_n: &str, types: Option<Vec<SymType>>) 
                              -> Vec<String> {
        let types = match types {
            Some(x) => { x },
            None    => vec![SymType::Var,
                            SymType::Binary,
                            SymType::Builtin, 
                            SymType::Environment]
        };

        let mut res: Vec<String> = Vec::new();

        if types.contains(&SymType::Var) {
            // check for Var symbol
            for scope in self.scopes.iter().rev() {
                for v in scope.vars.iter().filter(|&(x, _)| x.starts_with(sym_n)) {
                    res.push(v.1.val.clone());
                }
                if scope.is_gl || scope.is_fn {
                    break;
                }
            }

            // check global scope (this is done separately so we can break at is_fn)
            for v in self.scopes.first().unwrap().vars.iter().filter(|&(x, _)| x.starts_with(sym_n)) {
                res.push(v.1.val.clone());
            }
        }

        if types.contains(&SymType::Builtin) {
            // check for Builtin symbol
            for v in self.builtins.iter().filter(|&(x, _)| x.starts_with(sym_n)) {
                res.push(v.1.name.to_string());
            }
        }

        if types.contains(&SymType::Binary) {
            // check for Binary symbol by filename
            for v in self.bins.iter().filter(|&(x, _)| x.starts_with(sym_n)) {
                res.push(v.1.clone().file_name().unwrap()
                            .to_os_string().into_string().unwrap());
            }
        }

        res
    }


    pub fn resolve(&mut self, sym_n: &str) -> Option<Sym> {
        self.resolve_types(sym_n, None)
    }

    // TODO: env vars, no?
    pub fn resolve_types(&mut self, sym_n: &str, types: Option<Vec<SymType>>) 
                        -> Option<Sym> {

        let types = match types {
            Some(x) => { x },
            None    => vec![SymType::Var,
                            SymType::Binary,
                            SymType::Builtin, 
                            SymType::Environment]
        };

        if types.contains(&SymType::Var) {
            // check for Var symbol
            for scope in self.scopes.iter().rev() {
                if let Some(v) = scope.vars.get(sym_n) {
                    return Some(Sym::Var(v.val.clone()));
                }
                if scope.is_gl || scope.is_fn {
                    break;
                }
            }

            // check global scope (this is done separately so we can break at is_fn)
            if let Some(v) = self.scopes[0].vars.get(sym_n) {
                return Some(Sym::Var(v.val.clone()));
            }
        }

        if types.contains(&SymType::Environment) {
            if let Ok(e) = env::var(sym_n) {
                return Some(Sym::Environment(e));
            }
        }

        if types.contains(&SymType::Builtin) {
            // check for Builtin symbol
            if let Some(bi) = self.builtins.get(sym_n) {
                return Some(Sym::Builtin(bi.clone()));
            }
        }

        if types.contains(&SymType::Binary) {
            // check for Binary symbol by filename
            if let Some(bin_path) = self.bins.get(sym_n) {
                return Some(Sym::Binary(bin_path.clone()));
            }

            // Check for executable file by full path
            if let Ok(_) = fs::metadata(sym_n) {
                // FIXME: needs more sanity checking for good files
                return Some(Sym::Binary(path::PathBuf::from(sym_n)));
            }

            // Re-hash bins and check again
            // TODO: make re-hash optional, since it has a noticeable runtime.
            if let Some(bin_path) = self.hash_bins().bins.get(sym_n) {
                return Some(Sym::Binary(bin_path.clone()));
            }
        }

        None
    }
}
