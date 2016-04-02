use builtins;
use std::collections::HashMap;
use std::env;
use std::fs;
use std::path;

pub enum Sym {
    Binary(path::PathBuf),
    Builtin(builtins::Builtin)
}

// We can use 'static for builtins because that's what builtins are: static.
pub struct Symtable {
    bins: HashMap<String, path::PathBuf>,
    builtins: HashMap<&'static str, builtins::Builtin>
}

impl Symtable {
    pub fn new() -> Symtable {
        let mut st = Symtable {
            bins: HashMap::new(),
            builtins: builtins::Builtin::map()
        };

        st.hash_bins();

        st
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

    pub fn resolve(&mut self, sym_n: &str) -> Option<Sym> {
        // TODO: functions, variables, etc.
        if let Some(bi) = self.builtins.get(sym_n) {
            return Some(Sym::Builtin(bi.clone()));
        }

        // Check for Binary symbol by filename
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

        // fail
        None
    }
}
