use alloc::string::String;
use alloc::vec::Vec;
// remove duplicate / in path and . and ..
pub fn clean_path(path: &str) -> String {
    let s = path.split("/").collect::<Vec<&str>>();
    let mut stack = Vec::new();
    for i in s {
        if i == "." || i == "" {
            continue;
        } else if i == ".." {
            stack.pop();
        } else {
            stack.push(i);
        }
    }
    stack.join("/")
}

