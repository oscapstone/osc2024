use crate::println;

use super::utils::{clean_path, split_path};

use alloc::format;
use core::ptr::NonNull;

use alloc::{boxed::Box, string::String, vec::Vec};

pub struct VFS {
    mount: NonNull<dyn VnodeOperations>,
}

impl VFS {
    pub fn new(fs: Box<dyn FileSystemOperations>) -> Self {
        VFS {
            mount: fs.as_ref().mount(),
        }
    }

    pub fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>, path: &str) {
        unimplemented!("VFS::mount()");
    }

    pub fn lookup(&self, path: &str) -> Option<NonNull<dyn VnodeOperations>> {
        let path_vec = split_path(path);
        let vnode = unsafe { self.mount.as_ref() };
        vnode.lookup(&path_vec)
    }

    pub fn open(&mut self, path: &str) -> Option<File> {
        let vnode = self.lookup(path);
        match vnode {
            Some(vnode) => {
                let file = File {
                    vnode: vnode,
                    f_pos: 0,
                };
                Some(file)
            }
            None => {
                let path_vec = split_path(path);
                let parent_path_vec = path_vec[0..path_vec.len() - 1].to_vec();

                let mount = unsafe { self.mount.as_mut() };
                // create file
                let mut parent_vnode = match mount.lookup(&parent_path_vec) {
                    Some(parent_node) => parent_node,
                    None => {
                        return None;
                    }
                };
                let file_name = path_vec[path_vec.len() - 1];
                let new_vnode = unsafe { parent_vnode.as_mut() }.mkfile(file_name);
                match new_vnode {
                    Ok(new_vnode) => {
                        let file = File {
                            vnode: new_vnode,
                            f_pos: 0,
                        };
                        Some(file)
                    }
                    Err(_) => {
                        return None;
                    }
                }
            }
        }
    }

    pub fn ls(&self, path: &str) -> Result<Vec<String>, String> {
        let vnode = self.lookup(path);
        match vnode {
            Some(vnode) => {
                let vnode = unsafe { vnode.as_ref() };
                match vnode.list_dir() {
                    Some(vnodes) => Ok(vnodes
                        .iter()
                        .map(|vnode| {
                            let vnode = unsafe { vnode.as_ref() };
                            vnode.get_name()
                        })
                        .collect()),
                    None => {
                        return Err(format!("ls: {} is not dir", path));
                    }
                }
            }
            None => {
                return Err(format!(
                    "ls: cannot access '{}': No such file or directory",
                    path
                ));
            }
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

    pub fn mkdir(&mut self, path: &str) -> Result<(), String> {
        let path_vec = split_path(path);
        println!("mkdir: {:?}", path_vec);
        let vnode = unsafe { self.mount.as_mut().lookup(&path_vec) };
        match vnode {
            Some(_) => {
                return Err(format!(
                    "mkdir: cannot create directory '{}': File exists",
                    path
                ));
            }
            None => {
                let vnode = unsafe { self.mount.as_mut() };
                let parent_path_vec = path_vec[0..path_vec.len() - 1].to_vec();
                let parent = vnode.lookup(&parent_path_vec);
                let file_name = path_vec[path_vec.len() - 1];
                match parent {
                    Some(mut parent) => {
                        let parent = unsafe { parent.as_mut() };
                        parent.mkdir(file_name);
                    }
                    None => {
                        return Err(format!(
                            "mkdir: cannot create directory '{}': No such file or directory",
                            path
                        ));
                    }
                }
            }
        }
        Ok(())
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
    fn mkfile(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String>;
    fn mkdir(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String>;

    // replace current vnode with new vnode
    fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>, path_vec: Vec<String>);
    // recover vnode
    fn umount(&mut self);
    fn get_parent(&self) -> Option<NonNull<dyn VnodeOperations>>;
    fn get_name(&self) -> String;
    fn list_dir(&self) -> Option<Vec<NonNull<dyn VnodeOperations>>>;
}

pub trait FileOperations {
    fn read(&self, buf: &mut [u8], len: usize) -> usize;
    fn write(&self, buf: &[u8], len: usize) -> usize;
    fn open(&self) -> Option<File>;
    fn close(&self);
    fn seek(&self, offset: usize) -> usize;
}
