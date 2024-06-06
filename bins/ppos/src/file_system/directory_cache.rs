use alloc::{rc::Weak, string::String, vec::Vec};
use core::{any::Any, fmt::Debug};
use library::sync::mutex::Mutex;

use super::{inode::INodeOperation, super_block::SuperBlockOperation};

pub trait DirectoryEntryOperation: Debug + Any {
    fn parent(&self) -> Option<Weak<dyn DirectoryEntryOperation>>;

    fn set_parent(&self, parent: Option<Weak<dyn DirectoryEntryOperation>>);

    fn name(&self) -> String;

    fn set_name(&self, name: String);

    fn inode(&self) -> Weak<dyn INodeOperation>;

    fn add_child(&self, child: Weak<dyn DirectoryEntryOperation>);

    fn remove_child(&self, name: &str);

    fn children(&self) -> Vec<Weak<dyn DirectoryEntryOperation>>;

    fn super_block(&self) -> Weak<dyn SuperBlockOperation>;
}

#[derive(Debug)]
pub struct DirectoryEntry {
    parent: Mutex<Option<Weak<dyn DirectoryEntryOperation>>>,
    name: Mutex<String>,
    inode: Weak<dyn INodeOperation>,
    children: Mutex<Vec<Weak<dyn DirectoryEntryOperation>>>,
    super_block: Weak<dyn SuperBlockOperation>,
}

impl DirectoryEntry {
    pub const fn new(
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
        name: String,
        inode: Weak<dyn INodeOperation>,
        super_block: Weak<dyn SuperBlockOperation>,
    ) -> Self {
        Self {
            parent: Mutex::new(parent),
            name: Mutex::new(name),
            inode,
            children: Mutex::new(Vec::new()),
            super_block,
        }
    }
}

impl DirectoryEntryOperation for DirectoryEntry {
    fn parent(&self) -> Option<Weak<dyn DirectoryEntryOperation>> {
        self.parent.lock().unwrap().clone()
    }

    fn set_parent(&self, parent: Option<Weak<dyn DirectoryEntryOperation>>) {
        *self.parent.lock().unwrap() = parent;
    }

    fn name(&self) -> String {
        self.name.lock().unwrap().clone()
    }

    fn inode(&self) -> Weak<dyn INodeOperation> {
        self.inode.clone()
    }

    fn add_child(&self, child: Weak<dyn DirectoryEntryOperation>) {
        self.children.lock().unwrap().push(child);
    }

    fn children(&self) -> Vec<Weak<dyn DirectoryEntryOperation>> {
        self.children.lock().unwrap().clone()
    }

    fn set_name(&self, name: String) {
        *self.name.lock().unwrap() = name;
    }

    fn super_block(&self) -> Weak<dyn SuperBlockOperation> {
        self.super_block.clone()
    }

    fn remove_child(&self, name: &str) {
        let mut children = self.children.lock().unwrap();
        let i = children
            .iter()
            .position(|x| x.upgrade().unwrap().name() == name)
            .unwrap();
        children.remove(i);
    }
}
