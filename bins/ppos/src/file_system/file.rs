use core::fmt::Debug;

use alloc::rc::Rc;
use library::sync::mutex::Mutex;

use super::inode::INodeOperation;

#[derive(Debug)]
pub struct File {
    inode: Rc<dyn INodeOperation>,
    position: Mutex<usize>,
}

pub trait FileOperation: Debug {
    fn write(&self, buf: &[u8], len: usize) -> Result<usize, &'static str>;

    fn read(&self, buf: &mut [u8], len: usize) -> Result<usize, &'static str>;

    fn close(&self);

    fn position(&self) -> usize;

    fn set_position(&self, position: usize);

    fn inode(&self) -> Rc<dyn INodeOperation>;
}

impl File {
    pub const fn new(inode: Rc<dyn INodeOperation>) -> Self {
        Self {
            inode,
            position: Mutex::new(0),
        }
    }
}

impl FileOperation for File {
    fn write(&self, buf: &[u8], len: usize) -> Result<usize, &'static str> {
        unimplemented!()
    }

    fn read(&self, buf: &mut [u8], len: usize) -> Result<usize, &'static str> {
        unimplemented!()
    }

    fn position(&self) -> usize {
        *self.position.lock().unwrap()
    }

    fn set_position(&self, position: usize) {
        *self.position.lock().unwrap() = position;
    }

    fn inode(&self) -> Rc<dyn INodeOperation> {
        self.inode.clone()
    }

    fn close(&self) {}
}
