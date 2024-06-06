use alloc::{
    rc::{Rc, Weak},
    string::String,
};
use core::{any::Any, fmt::Debug};

use super::{
    directory_cache::DirectoryEntryOperation, file::FileOperation, super_block::SuperBlockOperation,
};

use library::time::Time;
use vfs::file::Umode;

pub trait INodeOperation: Debug + Any {
    fn lookup(
        &self,
        parent_directory: Rc<dyn DirectoryEntryOperation>,
        target_name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>>;

    fn create(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation>;

    fn mkdir(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation>;

    fn open(&self, inode: Rc<dyn INodeOperation>) -> Rc<dyn FileOperation>;

    fn size(&self) -> usize;

    fn super_block(&self) -> Weak<dyn SuperBlockOperation>;
}

#[derive(Debug)]
pub struct INode {
    umode: Umode,
    uid: u32,
    gid: u32,
    atime: Time,
    mtime: Time,
    ctime: Time,
    size: usize,
    super_block: Weak<dyn SuperBlockOperation>,
}

impl INode {
    pub const fn new(
        umode: Umode,
        uid: u32,
        gid: u32,
        atime: Time,
        mtime: Time,
        ctime: Time,
        size: usize,
        super_block: Weak<dyn SuperBlockOperation>,
    ) -> Self {
        Self {
            umode,
            uid,
            gid,
            atime,
            mtime,
            ctime,
            size,
            super_block,
        }
    }
}

impl INodeOperation for INode {
    fn lookup(
        &self,
        parent_directory: Rc<dyn DirectoryEntryOperation>,
        target_name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>> {
        for child in parent_directory.children() {
            if child.upgrade().unwrap().name() == target_name {
                return Some(child.upgrade().unwrap());
            }
        }
        None
    }

    fn create(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        unimplemented!()
    }

    fn mkdir(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        unimplemented!()
    }

    fn open(&self, inode: Rc<dyn INodeOperation>) -> Rc<dyn FileOperation> {
        unimplemented!()
    }

    fn size(&self) -> usize {
        self.size
    }

    fn super_block(&self) -> Weak<dyn SuperBlockOperation> {
        self.super_block.clone()
    }
}
