use super::util::clean_path;
use alloc::format;
use core::{num::ParseIntError, panic};

use alloc::{
    boxed::Box,
    collections::binary_heap::Iter,
    rc::{Rc, Weak},
    string::{String, ToString},
    vec::Vec,
};

use alloc::vec;

pub trait FileSystem {
    fn open(&self, path: &str) -> Result<FileHandle, String>;
    fn close(&self, file: FileHandle) -> Result<(), String>;
    
    fn mkdir(&self, path: &str) -> Result<(), String>;
    fn rmdir(&self, path: &str) -> Result<(), String>;
    fn ls(&self, path: &str) -> Result<Vec<String>, String>;
}

struct Node<T> {
    value: T,
    vec: Vec<Option<*mut T>>,
}

struct Directory {
    name: String,
    child: Vec<*mut VfsNode>,
}

impl Directory {
    fn new(name: &str) -> *mut Directory {
        let dir = Box::new(Directory {
            name: name.to_string(),
            child: Vec::new(),
        });
        Box::into_raw(dir)
    }
    fn rm_child(&mut self, child: *mut VfsNode) {
        self.child.retain(|c| *c != child);
    }
}

struct FileHandle {
    file: *mut File,
    offset: usize,
}

impl FileHandle {
    fn new(file: *mut File) -> Self {
        Self { file, offset: 0 }
    }
}

struct File {}

struct Mount {}

enum VfsNodeType {
    Directory(*mut Directory),
    File(*mut File),
    Mount(*mut Mount),
}

struct VfsNode {
    name: String,
    parent: Option<*mut VfsNode>,
    node_type: VfsNodeType,
}

impl VfsNode {
    fn new(node_type: VfsNodeType, name: &str) -> *mut VfsNode {
        let node = Box::new(VfsNode {
            parent: None,
            node_type,
            name: name.to_string(),
        });
        Box::into_raw(node)
    }

    pub fn push_child(&mut self, child: *mut VfsNode) -> Result<(), String> {
        match self.node_type {
            VfsNodeType::Directory(dir) => unsafe {
                child.as_mut().unwrap().parent = Some(self);
                (*dir).child.push(child);
                Ok(())
            },
            _ => Err(format!("{} is not a directory.", self.name)),
        }
    }

    pub fn get_child(&self, name: &str) -> Option<*mut VfsNode> {
        match self.node_type {
            VfsNodeType::Directory(dir) => {
                for c in unsafe { &(*dir).child } {
                    match unsafe { &(**c).node_type } {
                        VfsNodeType::Directory(d) => {
                            if unsafe { &(**d).name } == name {
                                return Some(*c);
                            }
                        }
                        _ => (),
                    }
                }
                None
            }
            _ => None,
        }
    }
}

pub struct Vfs {
    name: String,
    root_node: *mut VfsNode,
    seek: usize,
}

impl Vfs {
    pub fn new(name: &str) -> Self {
        Self {
            name: name.to_string(),
            root_node: Box::into_raw(Box::new(VfsNode {
                name: "".to_string(),
                parent: None,
                node_type: VfsNodeType::Directory(Directory::new("")),
            })),
            seek: 0,
        }
    }

    fn add_node(&mut self, node: *mut VfsNode) {
        let mut parent = unsafe { &*self.root_node };
        match parent.node_type {
            VfsNodeType::Directory(dir) => unsafe { (*dir).child.push(node) },
            _ => (),
        }
    }

    fn find_node_by_path(&self, path: &str) -> Option<*mut VfsNode> {
        let mut node = self.root_node;
        for name in path.split("/") {
            if name == "" {
                continue;
            }
            match unsafe { &*node }.get_child(name) {
                Some(child) => node = child,
                None => return None,
            }
        }
        Some(node)
    }
}

impl FileSystem for Vfs {
    fn open(&self, path: &str) -> Result<FileHandle, String> {
        let mut node = self.root_node;
        for name in path.split("/") {
            if name == "" {
                continue;
            }
            match unsafe { &*node }.get_child(name) {
                Some(child) => node = child,
                None => return Err(format!("{} not found", name)),
            }
        }
        match unsafe { &*node }.node_type {
            VfsNodeType::File(file) => Ok(FileHandle::new(file)),
            _ => Err(format!("{} is not a file", path)),
        }
    }

    fn close(&self, file: FileHandle) -> Result<(), String> {
        Ok(())
    }

    fn ls(&self, path: &str) -> Result<Vec<String>, String> {
        let path = clean_path(path);
        let node = self.find_node_by_path(&path);
        match node {
            Some(node) => match unsafe { &*node }.node_type {
                VfsNodeType::Directory(dir) => {
                    let mut res = Vec::new();
                    for child in unsafe { &(*dir).child } {
                        match unsafe { &**child }.node_type {
                            VfsNodeType::Directory(d) => res.push(unsafe { (*d).name.clone() }),
                            VfsNodeType::File(f) => res.push(unsafe { &*node }.name.clone()),
                            VfsNodeType::Mount(m) => res.push(unsafe { &*node }.name.clone()),
                        }
                    }
                    Ok(res)
                }
                VfsNodeType::File(f) => Ok(vec![unsafe { &*node }.name.clone()]),
                VfsNodeType::Mount(m) => Ok(vec![unsafe { &*node }.name.clone()]),
            },
            None => Err(format!("{} not found", path)),
        }
    }

    fn mkdir(&self, path: &str) -> Result<(), String> {
        let mut node = self.root_node;
        let path = clean_path(path);
        for name in path.split("/") {
            if name == "" {
                continue;
            }
            match unsafe { &*node }.get_child(name) {
                Some(child) => node = child,
                None => {
                    let new_node = VfsNode::new(VfsNodeType::Directory(Directory::new(name)), name);
                    match unsafe { (*node).push_child(new_node) } {
                        Ok(_) => node = new_node,
                        Err(e) => return Err(e),
                    }
                }
            }
        }
        Ok(())
    }

    fn rmdir(&self, path: &str) -> Result<(), String> {
        let path = clean_path(path);
        let node = self.find_node_by_path(&path);
        match node {
            Some(node) => match unsafe { &*node }.node_type {
                VfsNodeType::Directory(dir) => {
                    if unsafe { &(*dir).child }.len() == 0 {
                        let parent = unsafe { &*(*node).parent.unwrap() };
                        match parent.node_type {
                            VfsNodeType::Directory(p) => unsafe { (*p).rm_child(node) },
                            _ => {
                                panic!("Parent is not a directory")
                            }
                        }
                        unsafe { Box::from_raw(dir) };
                        Ok(())
                    } else {
                        Err("Directory is not empty".to_string())
                    }
                }
                _ => Err(format!("{} is not a directory", path)),
            },
            None => Err(format!("{} not found", path)),
        }
    }
}
