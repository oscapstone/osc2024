use crate::println;

use super::utils::{split_path};

use alloc::format;
use core::ptr::NonNull;

use alloc::{string::String, vec::Vec};

use alloc::string::ToString;

pub struct VFS {
    mount: NonNull<dyn VnodeOperations>,
    open_files: Vec<File>,
}

impl VFS {
    pub fn new(fs: NonNull<dyn FileSystemOperations>) -> Self {
        VFS {
            mount: unsafe {fs.as_ref()}.mount(),
            open_files: Vec::new(),
        }
    }

    pub fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>, path: &str) -> Result<(), String> {
        let vnode = self.lookup(path);
        
        match &vnode {
            Some(mut vnode) => {
                let vnode = unsafe { vnode.as_mut() };
                vnode.mount(fs);
                Ok(())
            }
            None => {
                Err(format!("mount: cannot find {}", path))
            }
        }
        
    }

    pub fn lookup(&self, path: &str) -> Option<NonNull<dyn VnodeOperations>> {
        let path_vec = split_path(path);
        let vnode = unsafe { self.mount.as_ref() };
        vnode.lookup(&path_vec)
    }

    pub fn open(&mut self, path: &str, is_create: bool) -> Result<File, String> {
        let vnode = self.lookup(path);
        match vnode {
            Some(vnode) => {
                let vnode = unsafe { vnode.as_ref() };
                match vnode.open() {
                    Ok(file) => Ok(File { file }),
                    Err(e) => Err(e),
                }
            }
            None => {
                if is_create{
                    let path_vec = split_path(path);
                    let parent_path_vec = path_vec[0..path_vec.len() - 1].to_vec();
    
                    let mount = unsafe { self.mount.as_mut() };
                    // create file
                    let mut parent_vnode = match mount.lookup(&parent_path_vec) {
                        Some(parent_node) => parent_node,
                        None => {
                            let parent_path = parent_path_vec.join("/");
                            return Err(format!("open: No such file or directory: {}", parent_path).to_string());
                        }
                    };
                    let file_name = &path_vec[path_vec.len() - 1];
                    let new_vnode = unsafe { parent_vnode.as_mut() }.mkfile(&file_name);
                    match new_vnode {
                        Ok(new_vnode) => {
                            let new_vnode = unsafe { new_vnode.as_ref() };
                            match new_vnode.open() {
                                Ok(file) => Ok(File { file }),
                                Err(e) => Err(e),
                            }
                        }
                        Err(e) => {
                            return Err(e);
                        }
                    }
                }
                else{
                    return Err(format!(
                        "open: cannot access '{}': No such file or directory",
                        path
                    ));
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

    

    pub fn read_fd(&self, file: &File, buf: &mut [u8], len: usize) -> usize {
        unimplemented!("VFS::read()")
    }   

    pub fn write_fd(&self, file: &File, buf: &[u8], len: usize) -> usize {
        unimplemented!("VFS::write()")
    }

    pub fn close_fd(&self, file: &File) {
        unimplemented!("VFS::close()")
    }

    pub fn mkdir(&mut self, path: &str) -> Result<(), String> {
        let path_vec = split_path(path);
        // println!("mkdir: {:?}", path_vec);
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
                let file_name = &path_vec[path_vec.len() - 1];
                match parent {
                    Some(mut parent) => {
                        let parent = unsafe { parent.as_mut() };
                        parent.mkdir(&file_name);
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
    
    pub fn umount(&mut self, path: &str) {
        let vnode = self.lookup(path);
        match vnode {
            Some(mut vnode) => {
                let vnode = unsafe { vnode.as_mut() };
                vnode.umount();
            }
            None => {
                println!("umount: cannot find {}", path);
            }
        }
    }
}

pub struct File {
    file: NonNull<dyn FileOperations>,
}

impl File {
    pub fn read(&mut self, len: usize) -> Result<Vec<u8>, String> {
        let file = unsafe { self.file.as_mut() };
        file.read(len)
    }

    pub fn write(&mut self, buf: &Vec<u8>) -> Result<usize, String> {
        let file = unsafe { self.file.as_mut() };
        file.write(buf)
    }

    pub fn seek(&mut self, offset: usize, whence: i64) -> Result<usize, String> {
        let file = unsafe { self.file.as_mut() };
        if whence == 0 {
            file.seek(offset)
        }
        else {
            unimplemented!("seek: whence != 0")
        }
    }

    pub fn close(&mut self) {
        let file = unsafe { self.file.as_mut() };
        file.close();
    }
}

pub trait FileSystemOperations {
    fn mount(&self) -> NonNull<dyn VnodeOperations>;
    fn get_name(&self) -> String;
}

pub trait VnodeOperations {
    fn lookup(&self, path_vec: &Vec<String>) -> Option<NonNull<dyn VnodeOperations>>;
    fn mkfile(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String>;
    fn mkdir(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String>;

    // replace current vnode with new vnode
    fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>);
    // recover vnode
    fn umount(&mut self);
    fn get_parent(&self) -> Option<NonNull<dyn VnodeOperations>>;
    fn get_name(&self) -> String;
    fn list_dir(&self) -> Option<Vec<NonNull<dyn VnodeOperations>>>;

    fn open(&self) -> Result<NonNull<dyn FileOperations>, String>;
}

pub trait FileOperations {
    fn read(&mut self, len: usize) -> Result<Vec<u8>, String>;
    fn write(&mut self, buf: &Vec<u8>) -> Result<usize, String>;
    fn seek(&mut self, offset: usize) -> Result<usize, String>;
    fn close(&mut self);
}
