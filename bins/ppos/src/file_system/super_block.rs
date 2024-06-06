use core::{any::Any, fmt::Debug};

use alloc::{
    rc::{Rc, Weak},
    vec::Vec,
};
use library::sync::mutex::Mutex;

use super::{
    directory_cache::DirectoryEntryOperation, file_system::FileSystemOperation,
    inode::INodeOperation,
};

pub trait SuperBlockOperation: Debug + Any {
    fn add_inode(&self, inode: Rc<dyn INodeOperation>);

    fn set_root(&self, root: Weak<dyn DirectoryEntryOperation>);

    fn root(&self) -> Option<Weak<dyn DirectoryEntryOperation>>;
}

#[derive(Debug)]
pub struct SuperBlock {
    inodes: Mutex<Vec<Rc<dyn INodeOperation>>>,
    root: Mutex<Option<Weak<dyn DirectoryEntryOperation>>>,
    file_system: Weak<dyn FileSystemOperation>,
}

impl SuperBlock {
    pub const fn new(file_system: Weak<dyn FileSystemOperation>) -> Self {
        Self {
            inodes: Mutex::new(Vec::new()),
            root: Mutex::new(None),
            file_system,
        }
    }
}

impl SuperBlockOperation for SuperBlock {
    fn add_inode(&self, inode: Rc<dyn INodeOperation>) {
        self.inodes.lock().unwrap().push(inode);
    }

    fn set_root(&self, root: Weak<dyn DirectoryEntryOperation>) {
        *self.root.lock().unwrap() = Some(root);
    }

    fn root(&self) -> Option<Weak<dyn DirectoryEntryOperation>> {
        self.root.lock().unwrap().clone()
    }
}
