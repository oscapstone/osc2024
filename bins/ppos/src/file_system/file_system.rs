use alloc::{
    boxed::Box,
    rc::{Rc, Weak},
    vec::Vec,
};
use core::{any::Any, fmt::Debug};
use library::sync::mutex::Mutex;

use super::{
    file_system_context::FileSystemContextOperation, super_block::SuperBlockOperation, VFSMount,
};

pub trait FileSystemOperation: Debug + Any {
    fn name(&self) -> &'static str;

    fn mount(
        &self,
        file_system: Weak<dyn FileSystemOperation>,
        device_name: &str,
    ) -> Result<VFSMount, &'static str>;

    fn init_file_system_context(
        &self,
        file_system: Weak<dyn FileSystemOperation>,
    ) -> Result<Box<dyn FileSystemContextOperation>, &'static str>;
}

#[derive(Debug)]
pub struct FileSystem {
    super_blocks: Mutex<Vec<Rc<dyn SuperBlockOperation>>>,
}

impl FileSystem {
    pub const fn new() -> Self {
        Self {
            super_blocks: Mutex::new(Vec::new()),
        }
    }

    pub fn add_super_block(&self, super_block: Rc<dyn SuperBlockOperation>) {
        self.super_blocks.lock().unwrap().push(super_block);
    }

    pub fn get_super_block(&self, index: usize) -> Option<Rc<dyn SuperBlockOperation>> {
        self.super_blocks.lock().unwrap().get(index).cloned()
    }
}
