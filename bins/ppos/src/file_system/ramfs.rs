use core::{any::Any, cmp::min};

use alloc::{
    boxed::Box,
    rc::{Rc, Weak},
    string::{String, ToString},
    vec::Vec,
};
use cpio::CPIOArchive;
use library::time::Time;
use vfs::file::{Umode, UMODE};

use crate::memory::{self, phys_to_virt};

use super::{
    directory_cache::{DirectoryEntry, DirectoryEntryOperation},
    file::{File, FileOperation},
    file_system::{FileSystem, FileSystemOperation},
    file_system_context::{FileSystemContext, FileSystemContextOperation},
    inode::{INode, INodeOperation},
    super_block::{SuperBlock, SuperBlockOperation},
    virtual_file_system, VFSMount,
};

#[derive(Debug)]
pub struct RamFS {
    inner: FileSystem,
}

impl RamFS {
    pub const fn new() -> Self {
        Self {
            inner: FileSystem::new(),
        }
    }
}

#[derive(Debug)]
pub struct RamFSContext {
    inner: FileSystemContext,
}

impl RamFSContext {
    pub const fn new(file_system: Weak<dyn FileSystemOperation>) -> Self {
        Self {
            inner: FileSystemContext::new(file_system),
        }
    }
}

impl FileSystemContextOperation for RamFSContext {
    fn get_tree(&self) -> Result<(), &'static str> {
        let super_block = Rc::new(RamFSSuperBlock::new(self.inner.file_system().clone()));
        let super_block_weak = Rc::downgrade(&super_block);
        let inode = Rc::new(RamFSINode::new(
            Umode::new(UMODE::OWNER_READ::SET),
            0,
            0,
            Time::new(0, 0),
            Time::new(0, 0),
            Time::new(0, 0),
            0,
            Rc::downgrade(&super_block),
            None,
        ));

        let root = Rc::new(RamFSDirectoryEntry::new(
            None,
            String::from("/"),
            Rc::downgrade(&(inode.clone() as Rc<dyn INodeOperation>)),
            super_block_weak.clone(),
        ));
        let root_weak = Rc::downgrade(&(root.clone() as Rc<dyn DirectoryEntryOperation>));
        virtual_file_system().add_directory_entry(root.clone());
        super_block.inner.add_inode(inode);
        super_block.inner.set_root(root_weak.clone());
        self.inner.set_root(root_weak.clone());
        let file_system = (self.inner.file_system().upgrade().unwrap() as Rc<dyn Any>)
            .downcast::<RamFS>()
            .unwrap();
        file_system.inner.add_super_block(super_block.clone());

        let mut devicetree = unsafe {
            devicetree::FlattenedDevicetree::from_memory(phys_to_virt(
                memory::DEVICETREE_START_ADDR,
            ))
        };
        devicetree
            .traverse(&|device_name, property_name, property_value| {
                if property_name == "linux,initrd-start" {
                    unsafe {
                        CPIO_ARCHIVE = CPIOArchive::from_memory(u32::from_be_bytes(
                            property_value.try_into().unwrap(),
                        ) as usize);
                    };
                    while let Some(file) = unsafe { CPIO_ARCHIVE.read_next() } {
                        if file.name == ".\0" {
                            continue;
                        }
                        let inode = Rc::new(RamFSINode::new(
                            file.metadata.umode.into(),
                            file.metadata.uid,
                            file.metadata.gid,
                            file.metadata.atime,
                            file.metadata.mtime,
                            file.metadata.ctime,
                            file.content.len(),
                            super_block_weak.clone(),
                            Some(unsafe {
                                core::slice::from_raw_parts(
                                    phys_to_virt(file.content.as_ptr() as usize) as *const u8,
                                    file.content.len(),
                                )
                            }),
                        ));
                        super_block.inner.add_inode(inode.clone());
                        let dentry = Rc::new(RamFSDirectoryEntry::new(
                            Some(root_weak.clone()),
                            file.name.strip_suffix("\0").unwrap().to_string(),
                            Rc::downgrade(&(inode.clone() as Rc<dyn INodeOperation>)),
                            super_block_weak.clone(),
                        ));
                        root.add_child(Rc::downgrade(
                            &(dentry.clone() as Rc<dyn DirectoryEntryOperation>),
                        ));
                        virtual_file_system().add_directory_entry(dentry);
                    }
                }
                Ok(())
            })
            .unwrap();

        Ok(())
    }
}

#[derive(Debug)]
pub struct RamFSFile {
    inner: File,
}

impl RamFSFile {
    pub const fn new(inode: Rc<RamFSINode>) -> Self {
        Self {
            inner: File::new(inode),
        }
    }
}

impl FileOperation for RamFSFile {
    fn write(&self, buf: &[u8], len: usize) -> Result<usize, &'static str> {
        unimplemented!()
    }

    fn read(&self, buf: &mut [u8], len: usize) -> Result<usize, &'static str> {
        let content = match Rc::downcast::<RamFSINode>(self.inner.inode())
            .unwrap()
            .content
        {
            Some(c) => c,
            None => return Err("No content in the inode"),
        };

        if content.len() == 0 {
            return Ok(0);
        }

        let start = self.inner.position();
        let readable_len = content.len() - start;
        let read_len = min(readable_len, len);
        let end = start + read_len;
        for i in start..end {
            buf[i] = content[i];
        }
        self.inner.set_position(end);
        Ok(read_len)
    }

    fn position(&self) -> usize {
        self.inner.position()
    }

    fn set_position(&self, position: usize) {
        self.inner.set_position(position)
    }

    fn inode(&self) -> Rc<dyn INodeOperation> {
        self.inner.inode()
    }

    fn close(&self) {}
}

#[derive(Debug)]
pub struct RamFSINode {
    inner: INode,
    content: Option<&'static [u8]>,
}

impl RamFSINode {
    pub const fn new(
        umode: Umode,
        uid: u32,
        gid: u32,
        atime: Time,
        mtime: Time,
        ctime: Time,
        size: usize,
        super_block: Weak<RamFSSuperBlock>,
        content: Option<&'static [u8]>,
    ) -> Self {
        Self {
            inner: INode::new(umode, uid, gid, atime, mtime, ctime, size, super_block),
            content,
        }
    }
}

impl INodeOperation for RamFSINode {
    fn create(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        self.inner.create(umode, name, parent)
    }

    fn mkdir(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        self.inner.mkdir(umode, name, parent)
    }

    fn lookup(
        &self,
        parent_directory: Rc<dyn DirectoryEntryOperation>,
        target_name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>> {
        self.inner.lookup(parent_directory, target_name)
    }

    fn open(&self, inode: Rc<dyn INodeOperation>) -> Rc<dyn FileOperation> {
        Rc::new(RamFSFile::new(Rc::downcast::<RamFSINode>(inode).unwrap()))
    }

    fn size(&self) -> usize {
        self.inner.size()
    }

    fn super_block(&self) -> Weak<dyn SuperBlockOperation> {
        self.inner.super_block()
    }
}

#[derive(Debug)]
pub struct RamFSSuperBlock {
    inner: SuperBlock,
}

impl RamFSSuperBlock {
    pub const fn new(file_system: Weak<dyn FileSystemOperation>) -> Self {
        Self {
            inner: SuperBlock::new(file_system),
        }
    }
}

impl RamFSSuperBlock {}

impl SuperBlockOperation for RamFSSuperBlock {
    fn add_inode(&self, inode: Rc<dyn INodeOperation>) {
        self.inner.add_inode(inode)
    }

    fn set_root(&self, root: Weak<dyn DirectoryEntryOperation>) {
        self.inner.set_root(root)
    }

    fn root(&self) -> Option<Weak<dyn DirectoryEntryOperation>> {
        self.inner.root()
    }
}

impl FileSystemOperation for RamFS {
    fn name(&self) -> &'static str {
        "RamFS"
    }

    fn mount(
        &self,
        file_system: Weak<dyn FileSystemOperation>,
        device_name: &str,
    ) -> Result<VFSMount, &'static str> {
        let fs_context = RamFSContext::new(file_system);
        fs_context.get_tree()?;
        Ok(VFSMount::new(
            fs_context.inner.root().unwrap().clone(),
            Rc::downgrade(&self.inner.get_super_block(0).unwrap()),
        ))
    }

    fn init_file_system_context(
        &self,
        file_system: Weak<dyn FileSystemOperation>,
    ) -> Result<Box<dyn FileSystemContextOperation>, &'static str> {
        Ok(Box::new(RamFSContext::new(file_system)))
    }
}

#[derive(Debug)]
pub struct RamFSDirectoryEntry {
    inner: DirectoryEntry,
}

impl RamFSDirectoryEntry {
    pub const fn new(
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
        name: String,
        inode: Weak<dyn INodeOperation>,
        super_block: Weak<dyn SuperBlockOperation>,
    ) -> Self {
        Self {
            inner: DirectoryEntry::new(parent, name, inode, super_block),
        }
    }
}

impl DirectoryEntryOperation for RamFSDirectoryEntry {
    fn name(&self) -> String {
        self.inner.name()
    }

    fn parent(&self) -> Option<Weak<dyn DirectoryEntryOperation>> {
        self.inner.parent()
    }

    fn inode(&self) -> Weak<dyn INodeOperation> {
        self.inner.inode()
    }

    fn add_child(&self, child: Weak<dyn DirectoryEntryOperation>) {
        self.inner.add_child(child)
    }

    fn set_parent(&self, parent: Option<Weak<dyn DirectoryEntryOperation>>) {
        self.inner.set_parent(parent)
    }

    fn children(&self) -> Vec<Weak<dyn DirectoryEntryOperation>> {
        self.inner.children()
    }

    fn set_name(&self, name: String) {
        self.inner.set_name(name)
    }

    fn super_block(&self) -> Weak<dyn SuperBlockOperation> {
        self.inner.super_block()
    }

    fn remove_child(&self, name: &str) {
        self.inner.remove_child(name)
    }
}

static mut CPIO_ARCHIVE: CPIOArchive = CPIOArchive::new();
