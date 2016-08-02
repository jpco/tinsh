pub struct Histvec {
    hist: Vec<String>,
    hpos: usize,
}

impl Histvec {
    pub fn new() -> Self {
        Histvec {
            hist: Vec::new(),
            hpos: 0,
        }
    }

    pub fn hist_up(&mut self) -> Option<&str> {
        if self.hpos < self.hist.len() {
            self.hpos = self.hpos + 1;
            Some(&self.hist[self.hpos - 1])
        } else {
            None
        }
    }

    pub fn hist_down(&mut self) -> Option<&str> {
        if self.hpos > 0 {
            self.hpos = self.hpos - 1;
            if self.hpos > 0 {
                Some(&self.hist[self.hpos - 1])
            } else {
                Some("")
            }
        } else {
            None
        }
    }

    pub fn hist_add(&mut self, nentry: &str) {
        if self.hist.len() == 0 || nentry != self.hist[0] {
            self.hist.insert(0, nentry.to_string());
        }
        self.hpos = 0;
    }

    pub fn hist_print(&self) {
        for elt in self.hist.iter().rev() {
            println!("  {}", elt);
        }
    }
}
