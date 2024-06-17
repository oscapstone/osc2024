use crate::println;

use super::vfs::{VnodeOperations, FileSystemOperations, FileOperations};
use super::utils::clean_path;

use alloc::ffi::NulError;
use alloc::format;
use core::borrow::BorrowMut;
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
    root: NonNull<TmpfsVnode>,
}

impl Tmpfs {
    pub fn new() -> Self {
        let root = Box::new(TmpfsVnode {
            name: "".to_string(),
            mount: None,
            parent: None,
            node_type: TmpfsVnodeType::Dir(Vec::new()),
            self_ref: None,
        });
        let root = Box::into_raw(root);
        let mut  root = NonNull::new(root).unwrap();
        unsafe {root.as_mut()}.self_ref = Some(root);
        Tmpfs {
            root: root,
        }
    }
}

impl FileSystemOperations for Tmpfs {
    fn mount(&self) -> NonNull<dyn VnodeOperations> {
        self.root
    }
}

enum TmpfsVnodeType {
    File,
    Dir(Vec<NonNull<TmpfsVnode>>),
}

struct TmpfsVnode {
    name: String,
    parent: Option<NonNull<dyn VnodeOperations>>,
    mount: Option<NonNull<dyn VnodeOperations>>,
    node_type: TmpfsVnodeType,
    self_ref: Option<NonNull<dyn VnodeOperations>>,
}

impl TmpfsVnode {
    fn add_child(&mut self, vnode: NonNull<TmpfsVnode>) -> Result<NonNull<dyn VnodeOperations>, String>{
        match &mut self.node_type {
            TmpfsVnodeType::Dir(children) => {
                children.push(vnode);
                Ok(vnode)
            },
            _ => {
                Err(format!("{} is not a directory", self.name))
            },
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
            },
            _ => None,
        }
    }

    fn get_name(&self) -> String {
        self.name.clone()
    }

    fn lookup(&self, path: &Vec<&str>) -> Option<NonNull<dyn VnodeOperations>> {
        println!("lookup: {:?}", path);
        if path.len() == 0 {
            return Some(self.self_ref.unwrap());
        }
        let name = path[0];
        match &self.node_type {
            TmpfsVnodeType::Dir(children) => {
                for child in children.iter() {
                    let child = unsafe {child.as_ref()};
                    if child.get_name() == name {
                        return child.lookup(&path[1..].to_vec());
                    }
                }
                return None;
            },
            _ => None,
        }
    }

    fn mkfile(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String> {
        let name = file_name;
        let vnode = Box::new(TmpfsVnode {
            name: name.to_string(),
            parent: Some(self.self_ref.unwrap()),
            mount: None,
            node_type: TmpfsVnodeType::File,
            self_ref: None,
        });
        let vnode = Box::into_raw(vnode);
        let mut vnode = NonNull::new(vnode).unwrap();
        unsafe {vnode.as_mut()}.self_ref = Some(vnode);
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
        });
        let vnode = Box::into_raw(vnode);
        let mut vnode = NonNull::new(vnode).unwrap();
        unsafe {vnode.as_mut()}.self_ref = Some(vnode);
        self.add_child(vnode)
    }

    fn umount(&mut self) {
        unimplemented!("TmpfsVnode::umount()");
    }

    fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>, path_vec: Vec<String>) {
        self.mount = Some(unsafe {fs.as_ref().mount()});
    }

    fn get_parent(&self) -> Option<NonNull<dyn VnodeOperations>> {
        self.parent
    }


}
