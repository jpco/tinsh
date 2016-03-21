use std::collections::HashMap;
use std::env;
use std::fs;
use std::path;

pub enum Sym {
    Binary(path::PathBuf)
}

pub struct Symtable {
    bins: HashMap<String, path::PathBuf>
}

impl Symtable {
    pub fn new() -> Symtable {
        Symtable {
            bins: HashMap::new()
        }
    }

    pub fn hash_bins(&mut self) {
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
    }

    pub fn resolve(&mut self, sym_n: &str) -> Option<Sym> {
        // TODO: functions, builtins, etc.

        // Check for Binary symbol by filename
        if let Some(bin_path) = self.bins.get(sym_n) {
            return Some(Sym::Binary(bin_path.clone()));
        }

        // Re-hash and check again
        // TODO: make re-hash optional, since it has a noticeable runtime.
        self.hash_bins();
        if let Some(bin_path) = self.bins.get(sym_n) {
            return Some(Sym::Binary(bin_path.clone()));
        }

        // Check for Binary symbol by full path
        let sym_bin = path::PathBuf::from(sym_n);
        if self.bins.values().find(|&x| x == (&sym_bin).as_path()).is_some() {
            return Some(Sym::Binary(sym_bin));
        }

        // fail
        None
    }
}
