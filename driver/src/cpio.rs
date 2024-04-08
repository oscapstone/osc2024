extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;
use crate::uart;

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

pub struct CpioHandler {
    cpio_start: *mut CpioNewcHeader,
    pub files: CpioIterator,
}

impl CpioHandler {
    pub fn new(cpio_start: *mut u8) -> CpioHandler {
        CpioHandler {
            cpio_start: cpio_start as *mut CpioNewcHeader,
            files: CpioIterator::new(&CpioHeaderWrapper::new(cpio_start as *mut CpioNewcHeader)),
        }
    }

    pub fn get_files(&self) -> CpioIterator {
        CpioIterator::new(&CpioHeaderWrapper::new(self.cpio_start))
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
    type Item = CpioFile;
    
    fn next(&mut self) -> Option<Self::Item> {
        if self.cur_header.get_file_name() == "TRAILER!!!" {
            return None;
        }
        let file_name = self.cur_header.get_file_name();
        let file_size = self.cur_header.get_file_size();
        let file_content = self.cur_header.get_current_file_content_ptr();
        let file = CpioFile::new(String::from(file_name), file_content, file_size);
        self.cur_header = self.cur_header.get_next_file();
        Some(file)
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

pub struct CpioFile {
    name: String,
    content: *const u8,
    size: usize,
    pos: usize,
}

impl CpioFile {
    pub fn new(name: String, content: *const u8, size: usize) -> CpioFile {
        CpioFile {
            name,
            content,
            size,
            pos: 0,
        }
    }

    pub fn read(&mut self, max_size: usize) -> &[u8] {
        let mut len = max_size;
        if self.pos + len > self.size {
            len = self.size - self.pos;
        }
        let slice = unsafe {core::slice::from_raw_parts(self.content.add(self.pos), len)};
        self.pos += len;
        slice
    }

    pub fn get_name(&self) -> &str {
        &self.name
    }

    pub fn get_size(&self) -> usize {
        self.size
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
