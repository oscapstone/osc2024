use alloc::string::String;
use alloc::string::ToString;
use alloc::vec::Vec;
// remove duplicate / in path and . and ..
pub fn clean_path(path: &str) -> String {
    path.split("/").filter(|name| *name != "").collect::<Vec<&str>>().join("/")
}

pub fn split_path(path: &str) -> Vec<String> {
    path.split("/").filter(|name| *name != "").map(|s| s.to_string()).collect::<Vec<String>>()
}

pub fn path_parser(path: &str) -> String {
    
    let path_vec = split_path(&path);
    let mut path_stack: Vec<String> = Vec::new();
    for name in path_vec {
        if name == ".." {
            path_stack.pop();
        } else if name != "." {

            path_stack.push(name.to_string());
        }
    }
    path_stack.join("/")
}
