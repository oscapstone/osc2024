extern crate alloc;

use alloc::string::{String, ToString};
use super::vfs::{FileOperations, FileSystemOperations, VnodeOperations};
use core::ptr::NonNull;
use alloc::vec::Vec;
use alloc::boxed::Box;

// a paraser to read cpio archive
#[repr(C, packed)]
struct CpioNewcHeader {
    c_magic: [u8; 6],
    c_ino: [u8; 8],
    c_mode: [u8; 8],
    c_uid: [u8; 8],
    c_gid: [u8; 8],
    c_nlink: [u8; 8],
    c_mtime: [u8; 8],
    c_filesize: [u8; 8],
    c_devmajor: [u8; 8],
    c_devminor: [u8; 8],
    c_rdevmajor: [u8; 8],
    c_rdevminor: [u8; 8],
    c_namesize: [u8; 8],
    c_check: [u8; 8],
}

pub struct CpioFS {
    name: String,
    cpio_start: *mut CpioNewcHeader,
    root_vnode: NonNull<dyn VnodeOperations>,
}

impl CpioFS {
    pub fn new(name: &str, cpio_start: *mut u8) -> CpioFS {
        CpioFS {
            name: name.to_string(),
            cpio_start: cpio_start as *mut CpioNewcHeader,
            root_vnode: CpioVnode::new_root("".to_string(), None, cpio_start),
        }
    }

    pub fn get_files(&self) -> CpioIterator {
        CpioIterator::new(&CpioHeaderWrapper::new(self.cpio_start))
    }
}

impl FileSystemOperations for CpioFS {
    fn mount(&self) -> NonNull<dyn VnodeOperations> {
        self.root_vnode
    }
    fn get_name(&self) -> String {
        self.name.clone()
    }
}

pub struct CpioIterator {
    cur_header: CpioHeaderWrapper,
}

impl CpioIterator {
    fn new(cpio_header: &CpioHeaderWrapper) -> CpioIterator {
        CpioIterator {
            cur_header: CpioHeaderWrapper::new(cpio_header.header),
        }
    }
}

impl Iterator for CpioIterator {
    type Item = CpioFileInternal;
    
    fn next(&mut self) -> Option<Self::Item> {
        if self.cur_header.get_file_name() == "TRAILER!!!" {
            return None;
        }
        let file_name = self.cur_header.get_file_name();
        let file_size = self.cur_header.get_file_size();
        let file_content = self.cur_header.get_current_file_content_ptr();
        let file = CpioFileInternal::new(String::from(file_name), file_content, file_size);
        self.cur_header = self.cur_header.get_next_file();
        Some(file)
    }
}

pub struct CpioVnode {
    name: String,
    parent: Option<NonNull<dyn VnodeOperations>>,
    vnode_type: CpioVnodeType,
    mount_vnode: Option<NonNull<dyn VnodeOperations>>,
}

enum CpioVnodeType {
    File(CpioFileInternal),
    Dir(Vec<NonNull<CpioVnode>>),
}

impl CpioVnode {
    pub fn new_root(name: String, parent: Option<NonNull<dyn VnodeOperations>>, cpio_start:*mut u8) -> NonNull<CpioVnode> {
        let b = Box::new(
        CpioVnode {
            name,
            parent: None,
            vnode_type: CpioVnodeType::Dir(Vec::new()),
            mount_vnode: None,
        });
        let mut vnode = NonNull::new(Box::into_raw(b)).unwrap();
        let children = CpioIterator::new(&CpioHeaderWrapper::new(cpio_start as *mut CpioNewcHeader)).map(|file| {
            let vnode = Box::new(CpioVnode {
                name: file.get_name().to_string(),
                parent: Some(vnode),
                vnode_type: CpioVnodeType::File(file),
                mount_vnode: None,
            });
            NonNull::new(Box::into_raw(vnode)).unwrap()
        });
        unsafe {vnode.as_mut()}.vnode_type = CpioVnodeType::Dir(children.collect());
        vnode
    }
}

impl VnodeOperations for CpioVnode {
    fn get_name(&self) -> String {
        self.name.clone()
    }

    fn get_parent(&self) -> Option<NonNull<dyn VnodeOperations>> {
        return self.parent;
    }


    fn list_dir(&self) -> Option<Vec<NonNull<dyn VnodeOperations>>> {
        match &self.vnode_type {
            CpioVnodeType::Dir(children) => {
                Some(children.iter().map(|child| {
                    NonNull::new(child.as_ptr() as *mut dyn VnodeOperations).unwrap()
                }).collect())
            }
            _ => None,
        }
    }

    fn lookup(&self, path_vec: &Vec<String>) -> Option<NonNull<dyn VnodeOperations>> {
        if let Some(mount) = self.mount_vnode {
            let mount = unsafe {mount.as_ref()};
            return mount.lookup(path_vec);
        }
        if path_vec.len() == 0 {
            return Some(NonNull::from(self));
        }
        let name = &path_vec[0];
        match &self.vnode_type {
            CpioVnodeType::Dir(children) => {
                for child in children.iter() {
                    let child = unsafe {child.as_ref()};
                    if child.get_name() == *name {
                        return child.lookup(&path_vec[1..].to_vec());
                    }
                }
                return None;
            }
            _ => return None,
        }
    }

    fn mkdir(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String> {
        if let Some(mut mount) = self.mount_vnode {
            let mount = unsafe {mount.as_mut()};
            return mount.mkdir(file_name);
        }
        unimplemented!("cpiofs: unsupported operation mkdir")
    }

    fn mkfile(&mut self, file_name: &str) -> Result<NonNull<dyn VnodeOperations>, String> {
        if let Some(mut mount) = self.mount_vnode {
            let mount = unsafe {mount.as_mut()};
            return mount.mkfile(file_name);
        }
        unimplemented!("cpiofs: unsupported operation mkdir")
    }
    fn mount(&mut self, fs: NonNull<dyn FileSystemOperations>) {
        self.mount_vnode = Some(unsafe {fs.as_ref()}.mount());
    }

    fn open(&self) -> Result<NonNull<dyn FileOperations>, String> {
        match &self.vnode_type {
            CpioVnodeType::File(file) => {
                let file = Box::new(CpioFile {
                    CpioFileInternal: file.clone(),
                    read_pos: 0,
                });
                Ok(NonNull::new(Box::into_raw(file) as *mut dyn FileOperations).unwrap())
            }
            _ => Err("cpiofs: cannot open a directory".to_string()),
        }
    }

    fn umount(&mut self) {
        unimplemented!("umount")
    }


}


struct CpioHeaderWrapper {
    header: *mut CpioNewcHeader,
}

impl CpioHeaderWrapper {
    pub fn new(header: *mut CpioNewcHeader) -> CpioHeaderWrapper {
        CpioHeaderWrapper {
            header,
        }
    }

    pub fn get_file_name(&self) -> &str {
        let namesize = unsafe {hex_to_u64(&(*self.header).c_namesize)};
        let name_ptr = (self.header as u64 + core::mem::size_of::<CpioNewcHeader>() as u64) as *mut u8;
        unsafe {
            core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                name_ptr,
                (namesize - 1) as usize,
            ))
        }
    }
    pub fn get_file_size(&self) -> usize {
        let filesize = unsafe {hex_to_u64(&(*self.header).c_filesize)};
        filesize as usize
    }
    pub fn get_next_file(&mut self) -> CpioHeaderWrapper {
        let mut offset = core::mem::size_of::<CpioNewcHeader>() as usize;
        let namesize = unsafe {hex_to_u64(&(*self.header).c_namesize)} as usize;
        offset += namesize;
        if offset % 4 != 0 {
            offset += 4 - offset % 4;
        }
        let filesize = unsafe {hex_to_u64(&(*self.header).c_filesize)} as usize;
        offset += filesize;
        if offset % 4 != 0 {
            offset += 4 - offset % 4;
        }
        self.header = ((self.header as u64) + offset as u64) as *mut CpioNewcHeader;
        CpioHeaderWrapper::new(self.header)
    }

    pub fn get_current_file_header(&self) -> *mut CpioNewcHeader{
        self.header
    }

    pub fn get_current_file_name_len(&self) -> usize {
        let namesize = unsafe {hex_to_u64(&(*self.header).c_namesize)};
        namesize as usize
    }

    pub fn get_current_file_content_ptr(&self) -> *const u8 {
        let cur_pos = self.header as *mut u8;
        let name_size = self.get_current_file_name_len();
        let mut offset = (core::mem::size_of::<CpioNewcHeader>()+ name_size) as u64 + cur_pos as u64 ;
        if offset % 4 != 0 {
            offset += 4 - offset % 4;
        }
        let file_ptr = offset as *mut u8;
        file_ptr
    }

    pub fn get_current_file_size(&self) -> usize {
        (unsafe {hex_to_u64(&(*self.header).c_filesize)} as usize)
    }
}

#[derive(Clone)]
pub struct CpioFileInternal {
    name: String,
    content: *const u8,
    size: usize,
}


impl CpioFileInternal {
    pub fn new(name: String, content: *const u8, size: usize) -> CpioFileInternal {
        CpioFileInternal {
            name,
            content,
            size,
        }
    }

    pub fn get_name(&self) -> &str {
        &self.name
    }

    pub fn get_size(&self) -> usize {
        self.size
    }
}

pub struct CpioFile {
    CpioFileInternal: CpioFileInternal,
    read_pos: usize,
}

impl FileOperations for CpioFile {
    fn read(&mut self, len: usize) -> Result<Vec<u8>, String> {
        if self.read_pos == self.CpioFileInternal.size {
            return Err("read: EOF".to_string());
        }
        let read_len = core::cmp::min(len, self.CpioFileInternal.size - self.read_pos);
        let read_data = unsafe {
            Vec::from_raw_parts(self.CpioFileInternal.content.add(self.read_pos) as *mut u8, read_len, read_len)
        };
        self.read_pos += read_len;
        Ok(read_data)
    }
    fn write(&mut self, buf: &Vec<u8>) -> Result<usize, String> {
        Err("write: not supported".to_string())
    }
    fn close(&mut self) {
        // do nothing
    }
    fn seek(&mut self, offset: usize) -> Result<usize, String> {
        if self.read_pos + offset > self.CpioFileInternal.size {
            return Err("file out of range.".to_string());
        }
        self.read_pos = self.read_pos + offset;
        Ok(offset)
    }
}

fn hex_to_u64(hex: &[u8; 8]) -> u64 {
    let mut result: u64 = 0;
    for i in 0..8 {
        result = result << 4;
        let c = hex[i];
        if c >= b'0' && c <= b'9' {
            result += (c - b'0') as u64;
        } else if c >= b'a' && c <= b'f' {
            result += (c - b'a' + 10) as u64;
        } else if c >= b'A' && c <= b'F' {
            result += (c - b'A' + 10) as u64;
        } else {
            break;
        }
    }
    result
}
