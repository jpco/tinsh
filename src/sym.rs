#![allow(dead_code)]

use builtins;
use std::collections::HashMap;
use std::env;
use std::fs;
use std::path;
use std::mem;

use opts;


#[derive(PartialEq, Copy, Clone)]
pub enum ScType {
    Fn,
    Loop,
    Global,
    Default,
}

#[derive(PartialEq, Copy, Clone)]
pub enum ScInter {
    Break,
    Continue,
    Return(i32),
}

pub enum Sym {
    Binary(path::PathBuf),
    Builtin(builtins::Builtin),
    Var(String),
    Environment(String),
    Fn(Fn),
}

pub enum SymE {
    Binary(path::PathBuf),
    Builtin(builtins::Builtin),
    Fn(Fn),
}

pub enum SymV {
    Var(String),
    Environment(String),
}

#[derive(Clone)]
pub struct Fn {
    pub name: String,
    pub inline: bool,
    pub args: Vec<String>,
    pub vararg: Option<String>,
    pub postargs: Option<Vec<String>>,
    pub lines: Vec<String>,
}

enum Val {
    Var(String),
    Fn(Fn),
}

// the 'is_fn' flag enables us to check
// whether the current scope is the top-level scope of
// a function -- aside from global ops, we don't want to act
// through the function barrier.
struct Scope {
    vars: HashMap<String, Val>, // contains vars and also functions
    sc_type: ScType,
    inter: Option<ScInter>,
}

#[derive(PartialEq, Copy, Clone)]
pub enum ScopeSpec {
    Local,
    Global,
    Environment,
    Default,
}


// We can use 'static for builtins because that's what builtins are: static.
pub struct Symtable {
    bins: HashMap<String, path::PathBuf>,
    builtins: HashMap<&'static str, builtins::Builtin>,
    scopes: Vec<Scope>,
    pub subsh: bool,
}

impl Symtable {
    pub fn new() -> Symtable {
        let mut st = Symtable {
            bins: HashMap::new(),
            builtins: builtins::Builtin::map(),
            scopes: Vec::new(),
            subsh: false,
        };

        st.scopes.push(Scope {
            vars: HashMap::new(),
            sc_type: ScType::Global,
            inter: None,
        });

        st.hash_bins();

        st
    }

    pub fn pull_sc_inter(&mut self) -> Option<ScInter> {
        mem::replace(&mut self.scopes.last_mut().unwrap().inter, None)
    }

    // NOTE: simply marks the relevant data structures, *does not* actually break/etc.
    pub fn sc_break(&mut self, mut d: u16) {
        let mut found = false;
        for mut sc in self.scopes.iter_mut().rev() {
            sc.inter = Some(ScInter::Break);
            if sc.sc_type == ScType::Loop {
                d -= 1;
            }
            if d == 0 {
                found = true;
                break;  // meta
            }
        }
        if !found {
            warn!("'break' command used without a loop (or enough loops)");

            // gorrammit, gotta reset all of them
            for mut sc in self.scopes.iter_mut() {
                sc.inter = None;
            }
        }
    }

    pub fn sc_continue(&mut self, mut d: u16) {
        let mut found = false;
        for mut sc in self.scopes.iter_mut().rev() {
            if sc.sc_type == ScType::Loop {
                d -= 1;
            }
            if d == 0 {
                found = true;
                sc.inter = Some(ScInter::Continue);
                break;  // soooo meta
            } else {
                sc.inter = Some(ScInter::Break);
            }
        }
        if !found {
            warn!("'continue' command used without a loop (or enough loops)");

            for mut sc in self.scopes.iter_mut() {
                sc.inter = None;
            }
        }
    }

    pub fn sc_return(&mut self, retcode: i32) {
        let mut found = false;
        for mut sc in self.scopes.iter_mut().rev() {
            sc.inter = Some(ScInter::Break);
            if sc.sc_type == ScType::Fn {
                found = true;
                sc.inter = Some(ScInter::Return(retcode));
                break;
            } else {
                sc.inter = Some(ScInter::Break);
            }
        }
        if !found {
            warn!("'return' command used not in a fn");

            for mut sc in self.scopes.iter_mut() {
                sc.inter = None;
            }
        }
    }

    pub fn set(&mut self, key: &str, val: String) -> Result<&mut Symtable, opts::OptError> {
        self.set_scope(key, val, ScopeSpec::Default)
    }

    fn scope_idx<'a>(&'a mut self, key: &str, sc: ScopeSpec) -> usize {
        match sc {
            ScopeSpec::Global => 0,
            ScopeSpec::Local => self.scopes.len() - 1,
            ScopeSpec::Environment => unreachable!(),
            ScopeSpec::Default => {
                let len = self.scopes.len();
                for (idx, scope) in self.scopes.iter_mut().rev().enumerate() {
                    if scope.vars.get(key).is_some() || scope.sc_type == ScType::Fn {
                        return len - idx - 1;
                    }
                }
                len - 1
            }
        }
    }

    pub fn set_fn(&mut self, key: &str, val: Fn, sc: ScopeSpec) -> &mut Symtable {
        {
            let idx = self.scope_idx(key, sc);
            let ref mut scope = self.scopes[idx];
            scope.vars.insert(key.to_string(), Val::Fn(val));
        }
        self
    }

    pub fn set_scope(&mut self,
                     key: &str,
                     val: String,
                     sc: ScopeSpec)
                     -> Result<&mut Symtable, opts::OptError> {
        if opts::is_opt(key) {
            try!(opts::set(key, val));
            return Ok(self);
        }

        if sc == ScopeSpec::Environment {
            env::set_var(key, val);
            return Ok(self);
        }

        // need to scope this for borrowck
        {
            let idx = self.scope_idx(key, sc);
            let ref mut scope = self.scopes[idx];

            if val == "" {
                scope.vars.remove(key);
            } else {
                scope.vars.insert(key.to_string(), Val::Var(val));
            }
        }

        Ok(self)
    }

    pub fn new_scope(&mut self, sc_type: ScType) -> &mut Symtable {
        self.scopes.push(Scope {
            vars: HashMap::new(),
            sc_type: sc_type,
            inter: None,
        });

        self
    }

    pub fn del_scope(&mut self) -> &mut Symtable {
        // error handling re: a bogus '}' is elsewhere
        self.scopes.pop();

        self
    }

    fn hash_bins(&mut self) -> &mut Symtable {
        self.bins.clear();

        let path_str = env::var("PATH").unwrap_or("/bin:/usr/bin".to_string());
        for path_dir in path_str.split(":") {
            if let Ok(path_dir) = fs::read_dir(path_dir) {
                for path_f in path_dir {
                    if let Err(e) = path_f {
                        warn!("Binary hash: {}", e);
                        break;
                    }
                    let path_f = path_f.unwrap();
                    let is_ok_f = match path_f.metadata() {
                        Ok(ent) => !ent.is_dir(),   // FIXME: should only be executable files
                        Err(_) => false,
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

    // TODO: make this happen (requires var compl)
    // pub fn prefix_resolve(&self, sym_n: &str) -> Vec<String> {
    // self.prefix_resolve_types(sym_n, None)
    // }


    // simply resolves a vector of matching strings.
    pub fn prefix_resolve_exec(&self, sym_n: &str) -> Vec<String> {
        let mut res: Vec<String> = Vec::new();

        // fns
        for scope in self.scopes.iter().rev() {
            for v in scope.vars.iter().filter(|&(x, _)| x.starts_with(sym_n)) {
                if let (n, &Val::Fn(_)) = v {
                    res.push(n.to_owned());
                }
            }
        }

        // builtins
        for v in self.builtins.iter().filter(|&(x, _)| x.starts_with(sym_n)) {
            res.push(v.1.name.to_string());
        }

        // binaries
        for v in self.bins.iter().filter(|&(x, _)| x.starts_with(sym_n)) {
            res.push(v.1
                .clone()
                .file_name()
                .unwrap()
                .to_os_string()
                .into_string()
                .unwrap());
        }

        res.sort();
        res.dedup();
        res
    }

    pub fn resolve_var(&self, sym_n: &str) -> Option<String> {
        if opts::is_opt(sym_n) {
            match opts::get(sym_n) {
                Some(s) => return Some(s),
                None => return None,
            }
        }

        // check for Var symbol
        for scope in self.scopes.iter().rev() {
            if let Some(&Val::Var(ref v)) = scope.vars.get(sym_n) {
                return Some(v.clone());
            }
        }

        None
    }

    pub fn resolve_fn(&self, sym_n: &str) -> Option<Fn> {
        for scope in self.scopes.iter().rev() {
            if let Some(&Val::Fn(ref f)) = scope.vars.get(sym_n) {
                return Some(f.clone());
            }
        }

        None
    }

    pub fn resolve_env(&self, sym_n: &str) -> Option<String> {
        match env::var(sym_n) {
            Ok(e) => Some(e),
            Err(_) => None,
        }
    }

    pub fn resolve_builtin(&self, sym_n: &str) -> Option<builtins::Builtin> {
        match self.builtins.get(sym_n) {
            Some(e) => Some(e.to_owned()),
            None => None,
        }
    }

    pub fn resolve_binary(&mut self, sym_n: &str) -> Option<path::PathBuf> {
        // check for Binary symbol by filename
        if let Some(bin_path) = self.bins.get(sym_n) {
            return Some(bin_path.clone());
        }

        // Check for executable file by full path
        if let Ok(_) = fs::metadata(sym_n) {
            // FIXME: needs more sanity checking for good files
            return Some(path::PathBuf::from(sym_n));
        }

        // Re-hash bins and check again
        // TODO: __tin_rehash option
        if !self.subsh {
            if let Some(bin_path) = self.hash_bins().bins.get(sym_n) {
                return Some(bin_path.clone());
            }
        }

        None
    }

    pub fn resolve_varish(&self, sym_n: &str) -> Option<SymV> {
        if let Some(res) = self.resolve_var(sym_n) {
            Some(SymV::Var(res))
        } else if let Some(res) = self.resolve_env(sym_n) {
            Some(SymV::Environment(res))
        } else {
            None
        }
    }

    pub fn resolve_exec(&mut self, sym_n: &str) -> Option<SymE> {
        if let Some(res) = self.resolve_fn(sym_n) {
            Some(SymE::Fn(res))
        } else if let Some(res) = self.resolve_builtin(sym_n) {
            Some(SymE::Builtin(res))
        } else if let Some(res) = self.resolve_binary(sym_n) {
            Some(SymE::Binary(res))
        } else {
            None
        }
    }

    pub fn resolve_any(&mut self, sym_n: &str) -> Option<Sym> {
        if let Some(res) = self.resolve_var(sym_n) {
            Some(Sym::Var(res))
        } else if let Some(res) = self.resolve_fn(sym_n) {
            Some(Sym::Fn(res))
        } else if let Some(res) = self.resolve_env(sym_n) {
            Some(Sym::Environment(res))
        } else if let Some(res) = self.resolve_builtin(sym_n) {
            Some(Sym::Builtin(res))
        } else if let Some(res) = self.resolve_binary(sym_n) {
            Some(Sym::Binary(res))
        } else {
            None
        }
    }
}
