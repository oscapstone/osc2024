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
    cpio_cur_pos: *mut CpioNewcHeader,
}

impl CpioHandler {
    // parse cpio in memory, with the start address of cpio

    pub fn new(cpio_start: *mut u8) -> CpioHandler {
        CpioHandler {
            cpio_start: cpio_start as *mut CpioNewcHeader,
            cpio_cur_pos: cpio_start as *mut CpioNewcHeader,
        }
    }

    fn get_current_file_header(&self) -> &CpioNewcHeader {
        unsafe { &*self.cpio_cur_pos }
    }

    pub fn is_end(&self) -> bool {
        let cur_header = self.get_current_file_header();
        let magic_slice = &cur_header.c_magic;
        let magic = core::str::from_utf8(magic_slice).unwrap();
        magic == "TRAILER!!"
    }

    pub fn get_current_file_name_len(&self) -> Option<usize> {
        if !self.is_end() {
            let cur_header = self.get_current_file_header();
            let namesize = hex_to_u64(&cur_header.c_namesize);
            Some(namesize as usize)
        } else {
            None
        }
    }

    pub fn get_current_file_size(&self) -> Option<usize> {
        if !self.is_end() {
            let cur_header = self.get_current_file_header();
            let filesize = hex_to_u64(&cur_header.c_filesize);
            Some(filesize as usize)
        } else {
            None
        }
    }

    fn get_current_header_magic(&self) -> &str {
        let cur_header = self.get_current_file_header();
        let magic_slice = &cur_header.c_magic;
        core::str::from_utf8(magic_slice).unwrap()
    }

    pub fn get_current_file_name(&self) -> Option<&str> {
        let cur_header = self.get_current_file_header();
        let namesize = hex_to_u64(&cur_header.c_namesize);
        let cur_pos = self.cpio_cur_pos as *mut u8;
        let name_ptr = (cur_pos as u64 + core::mem::size_of::<CpioNewcHeader>() as u64) as *mut u8;
        let name = unsafe {
            core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                name_ptr,
                (namesize - 1) as usize,
            ))
        };
        if name == "TRAILER!!!" {
            None
        } else {
            Some(name)
        }
    }

    pub fn read_current_file(&self) -> &[u8] {
        let cur_header = self.get_current_file_header();
        let filesize = hex_to_u64(&cur_header.c_filesize);
        let cur_pos = self.cpio_cur_pos as *mut u8;
        let name_size = hex_to_u64(&cur_header.c_namesize);
        let mut offset = core::mem::size_of::<CpioNewcHeader>() as u64 + name_size + cur_pos as u64;
        if offset % 4 != 0 {
            offset += 4 - offset % 4;
        }
        let file_ptr = offset as *mut u8;
        unsafe { core::slice::from_raw_parts(file_ptr, filesize as usize) }
    }

    pub fn next_file(&mut self) {
        let mut cur_pos = self.cpio_cur_pos as *mut u8;
        let cur_header = self.get_current_file_header();
        let mut offset = core::mem::size_of::<CpioNewcHeader>() as u64;
        let namesize = hex_to_u64(&cur_header.c_namesize);
        offset += namesize;
        if offset % 4 != 0 {
            offset += 4 - offset % 4;
        }
        let filesize = hex_to_u64(&cur_header.c_filesize);
        offset += filesize;
        if offset % 4 != 0 {
            offset += 4 - offset % 4;
        }
        cur_pos = (cur_pos as u64 + offset) as *mut u8;
        self.cpio_cur_pos = cur_pos as *mut CpioNewcHeader;
    }

    pub fn rewind(&mut self) {
        self.cpio_cur_pos = self.cpio_start;
    }

    pub fn list_all_files(&mut self) -> Vec<alloc::string::String> {
        uart::uart_write_str("Listing all files...\r\n");
        let mut files = Vec::new();
        loop {
            if let Some(name) = self.get_current_file_name() {
                files.push(String::from(name));
            } else {
                break;
            }
            self.next_file();
        }
        self.rewind();
        files
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
