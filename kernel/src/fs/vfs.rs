use super::utils::{clean_path, split_path};

use alloc::format;
use core::{num::ParseIntError, panic, ptr::NonNull};

use alloc::{
    boxed::Box,
    rc::{Rc, Weak},
    string::{String, ToString},
    vec::Vec,
};


pub struct VFS {
    mount: NonNull<dyn VnodeOperations>,
}

impl VFS {
    pub fn new(fs: Box<dyn FileSystemOperations>) -> Self {
        VFS {
            mount: unsafe {fs.as_ref().mount()},
        }
    }

    pub fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>, path: &str) {
        unimplemented!("VFS::mount()");
    }

    pub fn lookup(&self, path: &str) -> Option<NonNull<dyn VnodeOperations>> {
        let path_vec = split_path(path);
        let mut vnode = unsafe {self.mount.as_ref()};
        vnode.lookup(&path_vec)
    }

    pub fn open(&self, path: &str) -> Option<File> {
        let vnode = self.lookup(path);
        match vnode {
            Some(vnode) => unsafe {
                let file = File {
                    vnode: vnode,
                    f_pos: 0,
                };
                Some(file)
            },
            None => {
                // create file
                let path_vec = split_path(path);
                let mut vnode = unsafe {self.mount.as_ref()};
                vnode.create(&path_vec);
                let vnode = vnode.lookup(&path_vec).unwrap();
                unsafe {
                    let file = File {
                        vnode: vnode,
                        f_pos: 0,
                    };
                    Some(file)
                }                
            },
        }
    }

    pub fn ls(&self, path: &str) -> Result<Vec<String>, String> {
        let vnode = self.lookup(path);
        match vnode {
            Some(vnode) => unsafe {
                Ok(vnode.as_ref().ls())
            },
            None => {
                Err(format!("ls: cannot access '{}': No such file or directory", path))
            },
        }
    }

    pub fn read(&self, file: &File, buf: &mut [u8], len: usize) -> usize {
        unimplemented!("VFS::read()")
    }

    pub fn write(&self, file: &File, buf: &[u8], len: usize) -> usize {
        unimplemented!("VFS::write()")
    }

    pub fn close(&self, file: &File) {
        unimplemented!("VFS::close()")
    }

    pub fn mkdir(&self, path: &str) -> Result<(), String> {
        unimplemented!("VFS::mkdir()")
    }
}


struct File {
    vnode: NonNull<dyn VnodeOperations>,
    f_pos: usize,
}

pub trait FileSystemOperations {
    fn mount(&self) -> NonNull<dyn VnodeOperations>;
}



pub trait VnodeOperations {
    fn lookup(&self, path_vec: &Vec<&str>) -> Option<NonNull<dyn VnodeOperations>>;
    fn create(&self, path_vec: &Vec<&str>) -> Option<NonNull<dyn VnodeOperations>>;
    fn mkdir(&self, path_vec: Vec<String>) -> Option<NonNull<dyn VnodeOperations>>;
    // replace current vnode with new vnode
    fn mount(&self, fs: *mut dyn FileSystemOperations, path_vec: Vec<String>) -> Option<NonNull<dyn  VnodeOperations>>;
    // recover vnode
    fn umount(&self);
    fn get_parent(&self) -> Option<*mut dyn VnodeOperations>;
    fn ls(&self) -> Vec<String>;
}

pub trait FileOperations {
    fn read(&self, buf: &mut [u8], len: usize) -> usize;
    fn write(&self, buf: &[u8], len: usize) -> usize;
    fn open(&self) -> Option<File>;
    fn close(&self);
    fn seek(&self, offset: usize) -> usize;
}


