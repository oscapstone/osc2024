use alloc::string::String;
use alloc::vec::Vec;
// remove duplicate / in path and . and ..
pub fn clean_path(path: &str) -> String {
    path.split("/").filter(|name| *name != "").collect::<Vec<&str>>().join("/")
}

pub fn split_path(path: &str) -> Vec<&str> {
    path.split("/").filter(|name| *name != "").collect::<Vec<&str>>()
}

