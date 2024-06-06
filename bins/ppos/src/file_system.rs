use core::fmt::{self, Debug};

use alloc::{
    rc::{Rc, Weak},
    string::{String, ToString},
    vec,
    vec::Vec,
};
use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};
use file::FileOperation;
use hashbrown::HashMap;
use inode::INodeOperation;
use library::{collections::fixed_size_table::FixedSizeTable, println, sync::mutex::Mutex};
use vfs::file::{Umode, UMODE};

use crate::scheduler::current;

use self::{
    directory_cache::DirectoryEntryOperation, file_system::FileSystemOperation, ramfs::RamFS,
    tmpfs::TmpFS,
};

pub mod directory_cache;
pub mod file;
pub mod file_descriptor;
pub mod file_system;
pub mod file_system_context;
pub mod file_system_info;
pub mod inode;
pub mod path;
pub mod ramfs;
pub mod super_block;
pub mod tmpfs;

#[derive(Debug, Clone)]
pub struct VFSMount {
    pub root: Weak<dyn DirectoryEntryOperation>,
    pub super_block: Weak<dyn super_block::SuperBlockOperation>,
}

impl VFSMount {
    pub const fn new(
        root: Weak<dyn DirectoryEntryOperation>,
        super_block: Weak<dyn super_block::SuperBlockOperation>,
    ) -> Self {
        Self { root, super_block }
    }
}

#[derive(Debug)]
pub struct VirtualFileSystem {
    root: Mutex<Option<VFSMount>>,
    file_systems: Mutex<Vec<Rc<dyn FileSystemOperation>>>,
    // workaround for HashMap doesn't have a const constructor
    dcache_map: Mutex<Option<HashMap<String, Vec<Rc<dyn DirectoryEntryOperation>>>>>,
    // workaround for FixedSizeTable doesn't have a const constructor
    open_file_table: Mutex<Option<FixedSizeTable<Rc<dyn FileOperation>>>>,
}

impl VirtualFileSystem {
    const KERNEL_MAX_OPEN_FILES: usize = 1024;

    pub const fn new() -> Self {
        Self {
            root: Mutex::new(None),
            file_systems: Mutex::new(Vec::new()),
            dcache_map: Mutex::new(None),
            open_file_table: Mutex::new(None),
        }
    }

    pub fn init(&self) {
        *self.dcache_map.lock().unwrap() = Some(HashMap::new());
        *self.open_file_table.lock().unwrap() =
            Some(FixedSizeTable::new(Self::KERNEL_MAX_OPEN_FILES))
    }

    pub fn register_file_system(
        &self,
        file_system: Rc<dyn FileSystemOperation>,
    ) -> Result<(), &'static str> {
        unsafe { disable_kernel_space_interrupt() }
        self.file_systems.lock().unwrap().push(file_system);
        unsafe { enable_kernel_space_interrupt() }
        Ok(())
    }

    fn find_file_system(&self, name: &str) -> Option<Rc<dyn FileSystemOperation>> {
        unsafe { disable_kernel_space_interrupt() }
        let file_systems = self.file_systems.lock().unwrap();
        for fs in file_systems.iter() {
            if fs.name() == name {
                unsafe { enable_kernel_space_interrupt() }
                return Some(fs.clone());
            }
        }
        unsafe { enable_kernel_space_interrupt() }
        None
    }

    pub fn unregister_file_system(
        &self,
        file_system: Rc<dyn FileSystemOperation>,
    ) -> Result<(), &'static str> {
        unsafe { disable_kernel_space_interrupt() }
        let mut file_systems = self.file_systems.lock().unwrap();
        let index = match file_systems
            .iter()
            .position(|fs| Rc::ptr_eq(fs, &file_system))
        {
            Some(index) => index,
            None => {
                unsafe { enable_kernel_space_interrupt() }
                return Err("file system not found");
            }
        };
        file_systems.remove(index);
        unsafe { enable_kernel_space_interrupt() }
        Ok(())
    }

    pub fn add_directory_entry(&self, directory_entry: Rc<dyn DirectoryEntryOperation>) {
        let key = directory_entry.name().to_string();
        unsafe { disable_kernel_space_interrupt() }
        let mut dcache_map_cache = self.dcache_map.lock().unwrap();
        let dcache_map = dcache_map_cache.as_mut().unwrap();
        match dcache_map.get_mut(&key) {
            Some(entry) => entry.push(directory_entry),
            None => {
                dcache_map.insert(key, vec![directory_entry]);
            }
        }
        unsafe { enable_kernel_space_interrupt() }
    }

    pub fn find_directory_entry(
        &self,
        parent: &Option<Weak<dyn DirectoryEntryOperation>>,
        name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>> {
        unsafe { disable_kernel_space_interrupt() }
        let dcache_map_mutex = self.dcache_map.lock().unwrap();
        let dcache_map: &HashMap<String, Vec<Rc<dyn DirectoryEntryOperation>>> =
            dcache_map_mutex.as_ref().unwrap();
        match dcache_map.get(name) {
            Some(entrys) => {
                for entry in entrys {
                    if (parent.is_none() && entry.parent().is_none())
                        || (parent.is_some()
                            && entry.parent().is_some()
                            && entry
                                .parent()
                                .as_ref()
                                .unwrap()
                                .ptr_eq(parent.as_ref().unwrap()))
                    {
                        unsafe { enable_kernel_space_interrupt() }
                        return Some(entry.clone());
                    }
                }
                None
            }
            None => {
                unsafe { enable_kernel_space_interrupt() }
                None
            }
        }
    }

    pub fn remove_directory_entry(
        &self,
        parent: &Option<Weak<dyn DirectoryEntryOperation>>,
        name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>> {
        unsafe { disable_kernel_space_interrupt() }
        let mut dcache_map_mutex = self.dcache_map.lock().unwrap();
        let dcache_map = dcache_map_mutex.as_mut().unwrap();
        match dcache_map.get_mut(name) {
            Some(entrys) => {
                for (i, entry) in entrys.iter().enumerate() {
                    if (parent.is_none() && entry.parent().is_none())
                        || (parent.is_some()
                            && entry.parent().is_some()
                            && entry
                                .parent()
                                .as_ref()
                                .unwrap()
                                .ptr_eq(parent.as_ref().unwrap()))
                    {
                        let e = entrys.swap_remove(i);
                        unsafe { enable_kernel_space_interrupt() }
                        return Some(e);
                    }
                }
                None
            }
            None => {
                unsafe { enable_kernel_space_interrupt() }
                None
            }
        }
    }

    pub fn update_directory_entry(
        &self,
        old_name: &str,
        directory_entry: Rc<dyn DirectoryEntryOperation>,
    ) {
        let d = self
            .remove_directory_entry(&directory_entry.parent(), old_name)
            .unwrap();
        self.add_directory_entry(d)
    }

    pub fn root(&self) -> Option<VFSMount> {
        self.root.lock().unwrap().clone()
    }

    pub fn open(&self, inode: Rc<dyn INodeOperation>) -> Result<usize, &'static str> {
        let file = inode.open(inode.clone());
        let index = self
            .open_file_table
            .lock()
            .unwrap()
            .as_mut()
            .unwrap()
            .add(file.clone())?;
        Ok(index)
    }

    pub fn close(&self, index: usize) -> Result<(), &'static str> {
        let file = self
            .open_file_table
            .lock()
            .unwrap()
            .as_mut()
            .unwrap()
            .remove(index)?;
        file.close();
        Ok(())
    }

    pub fn get_file(&self, index: usize) -> Result<Rc<dyn FileOperation>, &'static str> {
        let file = self
            .open_file_table
            .lock()
            .unwrap()
            .as_ref()
            .unwrap()
            .get(index)?
            .clone();
        Ok(file)
    }

    pub fn lookup(&self, path: &str) -> Result<Rc<dyn DirectoryEntryOperation>, &'static str> {
        let mut path = path.to_string();
        let mut parent: Rc<dyn DirectoryEntryOperation>;
        if path.starts_with("/") {
            path = path[1..].to_string();
            parent = self.root().unwrap().root.upgrade().unwrap();
        } else {
            parent = unsafe { &*current() }
                .current_working_directory()
                .dentry
                .upgrade()
                .unwrap();
        }
        for name in path.split("/") {
            match name {
                "" | "." => continue,
                ".." => {
                    if let Some(p) = parent.parent() {
                        parent = p.upgrade().unwrap();
                    }
                }
                name => match self.find_directory_entry(&Some(Rc::downgrade(&parent)), name) {
                    Some(entry) => {
                        parent = entry;
                    }
                    None => {
                        return Err("no such file or directory");
                    }
                },
            }
        }
        Ok(parent)
    }

    pub fn get_file_system(&self, name: &str) -> Option<Rc<dyn FileSystemOperation>> {
        for fs in self.file_systems.lock().unwrap().iter() {
            if fs.name() == name {
                return Some(fs.clone());
            }
        }
        None
    }
}

impl fmt::Display for VirtualFileSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        writeln!(f, "FileSystemManager: {{")?;
        writeln!(f, "  root: {:?}", *self.root.lock().unwrap())?;
        writeln!(
            f,
            "  file systems: {:?}",
            *self.file_systems.lock().unwrap()
        )?;
        write!(f, "}}")
    }
}

pub fn init_root_file_system() -> Result<(), &'static str> {
    let file_system_manager = virtual_file_system();
    file_system_manager.init();
    let tmpfs = Rc::new(TmpFS::new());
    file_system_manager.register_file_system(tmpfs.clone())?;
    let ramfs = Rc::new(RamFS::new());
    file_system_manager.register_file_system(ramfs.clone())?;
    let root_mount = tmpfs.mount(
        Rc::downgrade(&tmpfs) as Weak<dyn FileSystemOperation>,
        "root",
    )?;
    let root = root_mount.root.upgrade().unwrap();
    let initramfs_mount = ramfs.mount(
        Rc::downgrade(&ramfs) as Weak<dyn FileSystemOperation>,
        "initramfs",
    )?;
    let initramfs_root = initramfs_mount.root.upgrade().unwrap();
    initramfs_root.set_name("initramfs".to_string());
    root.add_child(initramfs_mount.root.clone());
    initramfs_root.set_parent(Some(root_mount.root.clone()));
    virtual_file_system().update_directory_entry("/", initramfs_root);
    root.inode().upgrade().unwrap().mkdir(
        Umode::new(UMODE::OWNER_READ::SET),
        String::from("dev"),
        Some(Rc::downgrade(&root)),
    );
    *file_system_manager.root.lock().unwrap() = Some(root_mount);
    Ok(())
}

static VIRTUAL_FILE_SYSTEM: VirtualFileSystem = VirtualFileSystem::new();

pub fn virtual_file_system() -> &'static VirtualFileSystem {
    &VIRTUAL_FILE_SYSTEM
}
