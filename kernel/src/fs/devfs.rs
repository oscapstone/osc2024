use crate::println;

use super::vfs::{FileOperations, FileSystemOperations, VnodeOperations};
use alloc::boxed::Box;
use alloc::string::ToString;
use alloc::vec;
use alloc::vec::Vec;
use alloc::{ffi::NulError, string::String};
use core::ptr::NonNull;

use driver::uart;

pub struct Devfs {
    name: String,
    vnode: NonNull<dyn VnodeOperations>,
}

impl Devfs {
    pub fn new(name: &str) -> Self {
        let root = Box::new(DevfsVnode::new_root());
        let root = Box::into_raw(root);
        let root = core::ptr::NonNull::new(root).unwrap();
        Self {
            name: name.to_string(),
            vnode: root,
        }
    }

}

impl FileSystemOperations for Devfs {
    fn mount(&self) -> NonNull<dyn VnodeOperations> {
        self.vnode
    }

    fn get_name(&self) -> String {
        self.name.clone()
    }
}

enum DevfsVnodeType {
    Uart,
    Framebuffer,
    Dir(Vec<core::ptr::NonNull<DevfsVnode>>),
}

pub struct DevfsVnode {
    name: String,
    mount: Option<core::ptr::NonNull<dyn VnodeOperations>>,
    parent: Option<core::ptr::NonNull<dyn VnodeOperations>>,
    node_type: DevfsVnodeType,
    self_ref: Option<core::ptr::NonNull<DevfsVnode>>,
}

impl DevfsVnode {
    pub fn new_root() -> Self {
        let uart = Box::new(DevfsVnode {
            name: "uart".to_string(),
            mount: None,
            parent: None,
            node_type: DevfsVnodeType::Uart,
            self_ref: None,
        });
        let uart = Box::into_raw(uart);
        let mut uart = core::ptr::NonNull::new(uart).unwrap();
        unsafe {uart.as_mut()}.self_ref = Some(uart);
        let framebuffer = Box::new(DevfsVnode {
            name: "framebuffer".to_string(),
            mount: None,
            parent: None,
            node_type: DevfsVnodeType::Framebuffer,
            self_ref: None,
        });

        let framebuffer = Box::into_raw(framebuffer);
        let mut framebuffer = core::ptr::NonNull::new(framebuffer).unwrap();

        unsafe {framebuffer.as_mut()}.self_ref = Some(framebuffer);

        Self {
            name: "".to_string(),
            mount: None,
            parent: None,
            node_type: DevfsVnodeType::Dir(vec![uart, framebuffer]),
            self_ref: None,
        }
    }
}

impl VnodeOperations for DevfsVnode {
    fn get_name(&self) -> String {
        self.name.clone()
    }

    fn get_parent(&self) -> Option<core::ptr::NonNull<dyn VnodeOperations>> {
        self.parent
    }

    fn lookup(&self, path_vec: &Vec<String>) -> Option<core::ptr::NonNull<dyn VnodeOperations>> {
        if let Some(mount) = self.mount {
            let mount = unsafe { mount.as_ref() };
            return mount.lookup(path_vec);
        }

        if path_vec.len() == 0 {
            return Some(self.self_ref.unwrap());
        }
        let mut path_vec = path_vec.clone();
        let name = path_vec.remove(0);
        match &self.node_type {
            DevfsVnodeType::Dir(vnodes) => {
                for vnode in vnodes {
                    let vnode = unsafe { vnode.as_ref() };
                    if vnode.get_name() == name {
                        return vnode.lookup(&path_vec);
                    }
                }
                None
            }
            _ => None,
        }
    }

    fn list_dir(&self) -> Option<Vec<NonNull<dyn VnodeOperations>>> {
        match &self.node_type {
            DevfsVnodeType::Dir(children) => {
                let mut vnodes: Vec<NonNull<dyn VnodeOperations>> = Vec::new();
                for vnode in children.iter() {
                    vnodes.push(NonNull::from(*vnode));
                }
                Some(vnodes)
            }
            _ => None,
        }
    }

    fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>) {
        self.mount = Some(unsafe { fs.as_ref() }.mount());
    }

    fn mkdir(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String> {
        Err("devfs: cannot mkdir".to_string())
    }

    fn mkfile(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String> {
        Err("devfs: cannot mkfile".to_string())
    }

    fn open(&self) -> Result<NonNull<dyn FileOperations>, String> {
        let file = Box::new(DevFile {
            vnode: self.self_ref.unwrap(),
        });
        let file = Box::into_raw(file);
        let file = core::ptr::NonNull::new(file).unwrap();
        Ok(file)
    }

    fn umount(&mut self) {
        unimplemented!("devfs umount")
    }
}

pub struct DevFile {
    vnode: NonNull<DevfsVnode>,
}

impl FileOperations for DevFile {
    fn read(&mut self, len: usize) -> Result<Vec<u8>, String> {
        let vnode = unsafe { self.vnode.as_ref() };
        match vnode.node_type {
            DevfsVnodeType::Uart => {
                let mut buf = Vec::new();
                while buf.len() < len {
                    let c = unsafe {uart::read_u8()};
                    match c {
                        Some(c) => buf.push(c),
                        None => {}
                    }
                }
                Ok(buf)
            }
            DevfsVnodeType::Framebuffer => Err("devfs: cannot read framebuffer".to_string()),
            DevfsVnodeType::Dir(_) => Err("devfs: cannot read dir".to_string()),
        }
    }

    fn write(&mut self, buf: &Vec<u8>) -> Result<usize, String> {
        let vnode = unsafe { self.vnode.as_ref() };
        match vnode.node_type {
            DevfsVnodeType::Uart => {
                for i in buf.iter() {
                    uart::push_write_buf(*i);
                }
                uart::flush();
                Ok(buf.len())
            }
            DevfsVnodeType::Framebuffer => {
                unimplemented!("devfs write framebuffer")
            }
            DevfsVnodeType::Dir(_) => Err("devfs: cannot write dir".to_string()),
        }
    }

    fn close(&mut self) {
        unimplemented!("devfs close")
    }

    fn seek(&mut self, offset: usize) -> Result<usize, String> {
        unimplemented!("devfs seek")
    }
}
