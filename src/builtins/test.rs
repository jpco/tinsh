use std::io::BufReader;
use std::fs;
use std::rc::Rc;

use exec::Arg;
use exec::Redir;
use shell::Shell;

/*
 * stringy tests
 *  - if (x)
 *  - if (x) == y
 *  - if (x) != y
 * filey tests
 *  - if -e (x)
 *  - if -d (x)
 *  - if -f (x)
 * inty tests
 *  - if (x) [> | >= | < | <=] y
 * patterny tests
 *  - if (x) =~ y
 *  - if (x) !~ y
 */

pub fn test_main() -> 
    Rc<Fn(Vec<Arg>, &mut Shell, Option<BufReader<fs::File>>) -> i32> {
    Rc::new(|mut args: Vec<Arg>, sh: &mut Shell, _in: Option<BufReader<fs::File>>| -> i32 {
        if args.len() == 0 {
            return 1;
        }
    })
}
