use core::{any::Any, cmp::min};

use alloc::{
    boxed::Box,
    rc::{Rc, Weak},
    string::String,
    vec,
    vec::Vec,
};
use library::{sync::mutex::Mutex, time::Time};
use vfs::file::{Umode, UMODE};

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
pub struct TmpFS {
    inner: FileSystem,
}

impl TmpFS {
    pub const fn new() -> Self {
        Self {
            inner: FileSystem::new(),
        }
    }
}

#[derive(Debug)]
pub struct TmpFSContext {
    inner: FileSystemContext,
}

impl TmpFSContext {
    pub const fn new(file_system: Weak<dyn FileSystemOperation>) -> Self {
        Self {
            inner: FileSystemContext::new(file_system),
        }
    }
}

impl FileSystemContextOperation for TmpFSContext {
    fn get_tree(&self) -> Result<(), &'static str> {
        let super_block = Rc::new(TmpFSSuperBlock::new(self.inner.file_system().clone()));
        let inode = Rc::new(TmpFSINode::new(
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
        let root = Rc::new(TmpFSDirectoryEntry::new(
            None,
            String::from("/"),
            Rc::downgrade(&(inode.clone() as Rc<dyn INodeOperation>)),
            Rc::downgrade(&(super_block.clone() as Rc<dyn SuperBlockOperation>)),
        ));
        virtual_file_system().add_directory_entry(root.clone());
        super_block.inner.add_inode(inode);
        super_block.inner.set_root(Rc::downgrade(
            &(root.clone() as Rc<dyn DirectoryEntryOperation>),
        ));
        self.inner
            .set_root(Rc::downgrade(&(root as Rc<dyn DirectoryEntryOperation>)));
        let file_system = (self.inner.file_system().upgrade().unwrap() as Rc<dyn Any>)
            .downcast::<TmpFS>()
            .unwrap();
        file_system.inner.add_super_block(super_block);
        Ok(())
    }
}

#[derive(Debug)]
pub struct TmpFSFile {
    inner: File,
}

impl TmpFSFile {
    pub const fn new(inode: Rc<TmpFSINode>) -> Self {
        Self {
            inner: File::new(inode),
        }
    }
}

impl FileOperation for TmpFSFile {
    fn write(&self, buf: &[u8], len: usize) -> Result<usize, &'static str> {
        let inode = Rc::downcast::<TmpFSINode>(self.inner.inode()).unwrap();
        if inode.content.lock().unwrap().is_none() {
            let mut content = vec![0_u8; len].into_boxed_slice();
            for i in 0..len {
                content[i] = buf[i];
            }
            *inode.content.lock().unwrap() = Some(content);
            Ok(len)
        } else {
            let mut content_mutex = inode.content.lock().unwrap();
            let content = content_mutex.as_mut().unwrap();
            let start = self.inner.position();
            let writable_len = content.len() - start;
            let write_len = min(writable_len, len);
            let end = start + write_len;
            for i in start..end {
                content[i] = buf[i];
            }
            self.inner.set_position(end);
            Ok(write_len)
        }
    }

    fn read(&self, buf: &mut [u8], len: usize) -> Result<usize, &'static str> {
        let inode = Rc::downcast::<TmpFSINode>(self.inner.inode()).unwrap();
        let content_mutex = inode.content.lock().unwrap();
        let content = match content_mutex.as_ref() {
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
pub struct TmpFSINode {
    inner: INode,
    content: Mutex<Option<Box<[u8]>>>,
}

impl TmpFSINode {
    pub const fn new(
        umode: Umode,
        uid: u32,
        gid: u32,
        atime: Time,
        mtime: Time,
        ctime: Time,
        size: usize,
        super_block: Weak<TmpFSSuperBlock>,
        content: Option<Box<[u8]>>,
    ) -> Self {
        Self {
            inner: INode::new(umode, uid, gid, atime, mtime, ctime, size, super_block),
            content: Mutex::new(content),
        }
    }
}

impl INodeOperation for TmpFSINode {
    fn create(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        let inode = Rc::new(TmpFSINode::new(
            umode,
            0,
            0,
            Time::new(0, 0),
            Time::new(0, 0),
            Time::new(0, 0),
            0,
            Rc::downgrade(
                &Rc::downcast::<TmpFSSuperBlock>(
                    self.inner.super_block().upgrade().unwrap() as Rc<dyn Any>
                )
                .unwrap(),
            ),
            None,
        ));
        self.inner
            .super_block()
            .upgrade()
            .unwrap()
            .add_inode(inode.clone());
        let directory_entry = Rc::new(TmpFSDirectoryEntry::new(
            parent.clone(),
            name,
            Rc::downgrade(&(inode as Rc<dyn INodeOperation>)),
            self.inner.super_block(),
        ));
        if parent.is_some() {
            parent.unwrap().upgrade().unwrap().add_child(Rc::downgrade(
                &(directory_entry.clone() as Rc<dyn DirectoryEntryOperation>),
            ))
        }
        virtual_file_system().add_directory_entry(directory_entry.clone());
        directory_entry
    }

    fn mkdir(
        &self,
        umode: Umode,
        name: String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        let inode = Rc::new(TmpFSINode::new(
            umode,
            0,
            0,
            Time::new(0, 0),
            Time::new(0, 0),
            Time::new(0, 0),
            0,
            Rc::downgrade(
                &Rc::downcast::<TmpFSSuperBlock>(
                    self.inner.super_block().upgrade().unwrap() as Rc<dyn Any>
                )
                .unwrap(),
            ),
            None,
        ));
        self.inner
            .super_block()
            .upgrade()
            .unwrap()
            .add_inode(inode.clone());
        let directory_entry = Rc::new(TmpFSDirectoryEntry::new(
            parent.clone(),
            name,
            Rc::downgrade(&(inode as Rc<dyn INodeOperation>)),
            self.inner.super_block(),
        ));
        if parent.is_some() {
            parent.unwrap().upgrade().unwrap().add_child(Rc::downgrade(
                &(directory_entry.clone() as Rc<dyn DirectoryEntryOperation>),
            ))
        }
        virtual_file_system().add_directory_entry(directory_entry.clone());
        directory_entry
    }

    fn lookup(
        &self,
        parent_directory: Rc<dyn DirectoryEntryOperation>,
        target_name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>> {
        self.inner.lookup(parent_directory, target_name)
    }

    fn open(&self, inode: Rc<dyn INodeOperation>) -> Rc<dyn FileOperation> {
        Rc::new(TmpFSFile::new(Rc::downcast::<TmpFSINode>(inode).unwrap()))
    }

    fn size(&self) -> usize {
        self.inner.size()
    }

    fn super_block(&self) -> Weak<dyn SuperBlockOperation> {
        self.inner.super_block()
    }
}

#[derive(Debug)]
pub struct TmpFSSuperBlock {
    inner: SuperBlock,
}

impl TmpFSSuperBlock {
    pub const fn new(file_system: Weak<dyn FileSystemOperation>) -> Self {
        Self {
            inner: SuperBlock::new(file_system),
        }
    }
}

impl SuperBlockOperation for TmpFSSuperBlock {
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

impl FileSystemOperation for TmpFS {
    fn name(&self) -> &'static str {
        "tmpfs"
    }

    fn mount(
        &self,
        file_system: Weak<dyn FileSystemOperation>,
        device_name: &str,
    ) -> Result<VFSMount, &'static str> {
        let fs_context = TmpFSContext::new(file_system);
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
        Ok(Box::new(TmpFSContext::new(file_system)))
    }
}

#[derive(Debug)]
pub struct TmpFSDirectoryEntry {
    inner: DirectoryEntry,
}

impl TmpFSDirectoryEntry {
    pub const fn new(
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
        name: String,
        inode: Weak<dyn INodeOperation>,
        super_blcok: Weak<dyn SuperBlockOperation>,
    ) -> Self {
        Self {
            inner: DirectoryEntry::new(parent, name, inode, super_blcok),
        }
    }

    pub fn inner(&self) -> &DirectoryEntry {
        &self.inner
    }
}

impl DirectoryEntryOperation for TmpFSDirectoryEntry {
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
