use alloc::rc::Weak;

use super::{directory_cache::DirectoryEntryOperation, VFSMount};

#[derive(Debug, Clone)]
pub struct Path {
    pub mount: VFSMount,
    pub dentry: Weak<dyn DirectoryEntryOperation>,
}

impl Path {
    pub const fn new(mount: VFSMount, dentry: Weak<dyn DirectoryEntryOperation>) -> Self {
        Self { mount, dentry }
    }
}
