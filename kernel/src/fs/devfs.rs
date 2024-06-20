use alloc::string::String;
use alloc::vec::Vec;
use alloc::boxed::Box;
use super::vfs::{FileOperations, FileSystemOperations, VnodeOperations};
use alloc::string::ToString;
use core::ptr::NonNull;

struct Devfs {
    name: String,
}

impl FileSystemOperations for Devfs {
    fn mount(&self) -> core::ptr::NonNull<dyn VnodeOperations> {
        unimplemented!("devfs mount")
    }

    fn get_name(&self) -> String {
        self.name.clone()
    }
}

enum DevfsVnodeType {
    File,
    Dir(Vec<core::ptr::NonNull<DevfsVnode>>),
}

pub struct DevfsVnode {
    name: String,
    mount: Option<core::ptr::NonNull<dyn VnodeOperations>>,
    parent: Option<core::ptr::NonNull<DevfsVnode>>,
    node_type: DevfsVnodeType,
    self_ref: Option<core::ptr::NonNull<DevfsVnode>>,
}

impl DevfsVnode {
    pub fn new(name: &str, node_type: DevfsVnodeType) -> Self {
        DevfsVnode {
            name: name.to_string(),
            mount: None,
            parent: None,
            node_type,
            self_ref: None,
        }
    }
}

impl VnodeOperations for DevfsVnode {
    fn get_name(&self) -> String {
        self.name.clone()
    }

    fn get_parent(&self) -> Option<core::ptr::NonNull<dyn VnodeOperations>> {
        unimplemented!("devfs get_parent")
    }

    fn lookup(&self, path_vec: &Vec<String>) -> Option<core::ptr::NonNull<dyn VnodeOperations>> {
        unimplemented!("devfs lookup")
    }

    fn list_dir(&self) -> Option<Vec<core::ptr::NonNull<dyn VnodeOperations>>> {
        unimplemented!("devfs list_dir")
    }

    fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>) {
        self.mount = Some(fs);
    }
}
