use crate::println;

use super::utils::clean_path;
use super::vfs::{FileOperations, FileSystemOperations, VnodeOperations};

use alloc::ffi::NulError;
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use core::any::Any;
use core::borrow::BorrowMut;
use core::cmp::min;
use core::ptr::NonNull;

use alloc::boxed::Box;
use alloc::string::ToString;

use alloc::vec;

const PAGE_SIZE: usize = 4096;

pub struct Tmpfs {
    name: String,
    root: NonNull<TmpfsVnode>,
}

impl Tmpfs {
    pub fn new(name: &str) -> Self {
        let root = Box::new(TmpfsVnode {
            name: "".to_string(),
            mount: None,
            parent: None,
            node_type: TmpfsVnodeType::Dir(Vec::new()),
            self_ref: None,
            file_size: 0,
        });
        let root = Box::into_raw(root);
        let mut root = NonNull::new(root).unwrap();
        unsafe { root.as_mut() }.self_ref = Some(root);
        Tmpfs {
            name: name.to_string(),
            root: root,
        }
    }
}

impl FileSystemOperations for Tmpfs {
    fn mount(&self) -> NonNull<dyn VnodeOperations> {
        self.root
    }
    fn get_name(&self) -> String {
        self.name.clone()
    }
}

enum TmpfsVnodeType {
    File(FileAddressManager),
    Dir(Vec<NonNull<TmpfsVnode>>),
}

struct FileAddressManager {
    addr: *mut u8,
    len: usize,
}

impl FileAddressManager {
    fn alloc(page_num: usize) -> Self {
        let mut page_alloc = unsafe { &mut crate::PAGE_ALLOC };
        let addr = page_alloc.page_alloc(page_num);
        FileAddressManager {
            addr: addr,
            len: page_num * PAGE_SIZE,
        }
    }

    fn extend_and_copy(&mut self, page_num: usize) {
        let mut page_alloc = unsafe { &mut crate::PAGE_ALLOC };
        let new_addr = page_alloc.page_alloc(self.len / PAGE_SIZE + page_num);
        let new_len = self.len + page_num * PAGE_SIZE;
        unsafe {
            core::ptr::copy(self.addr, new_addr, self.len);
        }
        self.len = new_len;
        self.addr = new_addr;
        // dealloc old addr
    }
}

struct TmpfsVnode {
    name: String,
    parent: Option<NonNull<dyn VnodeOperations>>,
    mount: Option<NonNull<dyn VnodeOperations>>,
    node_type: TmpfsVnodeType,
    file_size: usize,
    self_ref: Option<NonNull<TmpfsVnode>>,
}

struct TmpfsFile {
    vnode: NonNull<TmpfsVnode>,
    f_read_pos: usize,
    f_write_pos: usize,
}

impl FileOperations for TmpfsFile {
    fn read(&mut self, len: usize) -> Result<Vec<u8>, String> {
        let vnode = unsafe { self.vnode.as_ref() };
        match &vnode.node_type {
            TmpfsVnodeType::File(file_addr_mgr) => {
                if self.f_read_pos == vnode.file_size {
                    return Err("read: EOF".to_string());
                }
                let read_len = min(len, vnode.file_size - self.f_read_pos);
                let read_data = Ok(unsafe {
                    Vec::from_raw_parts(file_addr_mgr.addr.add(self.f_read_pos), read_len, read_len)
                });
                self.f_read_pos += read_len;

                read_data
            }
            _ => Err(format!("{} is not a file", vnode.name)),
        }
    }

    fn write(&mut self, buf: &Vec<u8>) -> Result<usize, String> {
        let vnode = unsafe { self.vnode.as_mut() };

        match &mut vnode.node_type {
            TmpfsVnodeType::File(file_addr_mgr) => {
                if vnode.file_size + buf.len() > file_addr_mgr.len {
                    file_addr_mgr.extend_and_copy(1);
                }
                unsafe {
                    core::ptr::copy(
                        buf.as_ptr(),
                        file_addr_mgr.addr.add(self.f_write_pos),
                        buf.len(),
                    );
                }
                vnode.file_size += buf.len();
                Ok(buf.len())
            }
            TmpfsVnodeType::Dir(_) => Err(format!("write: {} is not a file", vnode.name)),
        }
    }

    fn seek(&mut self, offset: usize) -> Result<usize, String> {
        let vnode = unsafe { self.vnode.as_ref() };
        match &vnode.node_type {
            TmpfsVnodeType::File(_) => {
                if self.f_read_pos + offset > vnode.file_size {
                    Err(format!("file out of range."))
                } else {
                    self.f_read_pos = self.f_read_pos + offset;
                    Ok(offset)
                }
            }
            TmpfsVnodeType::Dir(_) => Err(format!("{} is not a file.", vnode.name)),
        }
    }

    fn close(&mut self) {}
}

impl TmpfsVnode {
    fn add_child(
        &mut self,
        vnode: NonNull<TmpfsVnode>,
    ) -> Result<NonNull<dyn VnodeOperations>, String> {
        match &mut self.node_type {
            TmpfsVnodeType::Dir(children) => {
                children.push(vnode);
                Ok(vnode)
            }
            _ => Err(format!("{} is not a directory", self.name)),
        }
    }
}

impl VnodeOperations for TmpfsVnode {
    fn list_dir(&self) -> Option<Vec<NonNull<dyn VnodeOperations>>> {
        match &self.node_type {
            TmpfsVnodeType::Dir(children) => {
                let mut vnodes: Vec<NonNull<dyn VnodeOperations>> = Vec::new();
                for child in children.iter() {
                    vnodes.push(NonNull::from(*child));
                }
                Some(vnodes)
            }
            _ => None,
        }
    }

    fn get_name(&self) -> String {
        self.name.clone()
    }

    fn lookup(&self, path: &Vec<String>) -> Option<NonNull<dyn VnodeOperations>> {
        match self.mount {
            Some(mount) => {
                if path.len() == 0 {
                    return Some(self.mount.unwrap());
                }
                let name = &path[0];
                let mount = unsafe { mount.as_ref() };
                return mount.lookup(path);
            }
            None => {
                if path.len() == 0 {
                    return Some(self.self_ref.unwrap());
                }
                let name = &path[0];
                match &self.node_type {
                    TmpfsVnodeType::Dir(children) => {
                        for child in children.iter() {
                            let child = unsafe { child.as_ref() };
                            if child.get_name() == *name {
                                return child.lookup(&path[1..].to_vec());
                            }
                        }
                        return None;
                    }
                    _ => return None,
                }
            },
        };
    }

    fn mkfile(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String> {
        let name = file_name;

        let file_addr_mgr = FileAddressManager::alloc(1);

        let vnode = Box::new(TmpfsVnode {
            name: name.to_string(),
            parent: Some(self.self_ref.unwrap()),
            mount: None,
            node_type: TmpfsVnodeType::File(file_addr_mgr),
            self_ref: None,
            file_size: 0,
        });
        let vnode = Box::into_raw(vnode);
        let mut vnode = NonNull::new(vnode).unwrap();
        unsafe { vnode.as_mut() }.self_ref = Some(vnode);
        self.add_child(vnode)
    }

    fn mkdir(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String> {
        let name = file_name;
        let vnode = Box::new(TmpfsVnode {
            name: name.to_string(),
            parent: Some(self.self_ref.unwrap()),
            mount: None,
            node_type: TmpfsVnodeType::Dir(Vec::new()),
            self_ref: None,
            file_size: 0,
        });
        let vnode = Box::into_raw(vnode);
        let mut vnode = NonNull::new(vnode).unwrap();
        unsafe { vnode.as_mut() }.self_ref = Some(vnode);
        self.add_child(vnode)
    }

    fn umount(&mut self) {
        unimplemented!("TmpfsVnode::umount()");
    }

    fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>) {
        self.mount = Some(unsafe { fs.as_ref().mount() });
    }

    fn get_parent(&self) -> Option<NonNull<dyn VnodeOperations>> {
        self.parent
    }

    fn open(&self) -> Result<NonNull<dyn FileOperations>, String> {
        let file = TmpfsFile {
            vnode: self.self_ref.unwrap(),
            f_read_pos: 0,
            f_write_pos: 0,
        };
        // println!("open: file_size: {}", self.file_size);
        let file = Box::new(file);
        let file = Box::into_raw(file) as *mut dyn FileOperations;
        Ok(NonNull::new(file).unwrap())
    }
}
