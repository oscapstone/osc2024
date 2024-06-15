use super::vfs::{VnodeOperations, FileSystemOperations, FileOperations};
use super::utils::clean_path;

use alloc::format;
use core::ptr::NonNull;
use alloc::string::String;
use alloc::vec::Vec;

use alloc::boxed::Box;
use alloc::string::ToString;


use alloc::vec;


struct File {
    vnode: NonNull<dyn VnodeOperations>,
    f_pos: usize,
}

pub struct Tmpfs {
    root: NonNull<dyn VnodeOperations>,
}

impl Tmpfs {
    pub fn new() -> Self {
        let root = Box::new(TmpfsVnode {
            name: "/".to_string(),
            parent: None,
            children: Vec::new(),
        });
        let root = Box::into_raw(root);
        Tmpfs {
            root: NonNull::new(root).unwrap(),
        }
    }
}

impl FileSystemOperations for Tmpfs {
    fn mount(&self) -> NonNull<dyn VnodeOperations> {
        self.root
    }
}

struct TmpfsVnode {
    name: String,
    parent: Option<NonNull<dyn VnodeOperations>>,
    children: Vec<NonNull<dyn VnodeOperations>>,
}

impl VnodeOperations for TmpfsVnode {
    fn lookup(&self, path: &Vec<&str>) -> Option<NonNull<dyn VnodeOperations>> {
        unimplemented!("TmpfsVnode::lookup()");
    }

    fn create(&self, path: &Vec<&str>) -> Option<NonNull<dyn VnodeOperations>> {
        unimplemented!("TmpfsVnode::create()");
    }

    fn mkdir(&self, path_vec: Vec<String>) -> Option<NonNull<dyn VnodeOperations>> {
        unimplemented!("TmpfsVnode::mkdir()");
    }

    fn umount(&self) {
        unimplemented!("TmpfsVnode::umount()");
    }

    fn mount(&self, fs: *mut dyn super::vfs::FileSystemOperations, path_vec: vec::Vec<alloc::string::String>) -> Option<NonNull<dyn  VnodeOperations>> {
        unimplemented!("TmpfsVnode::mount()");
    }

    fn get_parent(&self) -> Option<*mut dyn VnodeOperations> {
        unimplemented!("TmpfsVnode::get_parent()");
    }

    fn ls(&self) -> vec::Vec<alloc::string::String> {
        unimplemented!("TmpfsVnode::ls()");
    }

}
