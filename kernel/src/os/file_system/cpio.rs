use crate::println;

use crate::os::stdio::*;
use alloc::vec::Vec;

pub enum CpioHeaderField {
    Magic,
    Ino,
    Mode,
    Uid,
    Gid,
    Nlink,
    Mtime,
    Filesize,
    Devmajor,
    Devminor,
    Rdevmajor,
    Rdevminor,
    Namesize,
    Check,
}

pub struct CpioArchive {
    header_ptr: *const u8,
}

impl CpioArchive {
    pub fn load(address: *const u8) -> CpioArchive {
        CpioArchive {
            header_ptr: address,
        }
    }

    pub fn get_file_list(&self) -> Vec<&str> {
        let mut files = Vec::new();
        let mut current_ptr = self.header_ptr;
        loop {
            files.push(self.get_file_name(current_ptr));

            match self.get_next_file_ptr(current_ptr) {
                Some(ptr) => current_ptr = ptr,
                None => break,
            }
        }
        files
    }

    pub fn print_file_list(&self) {
        let mut current_ptr = self.header_ptr;
        loop {
            println(self.get_file_name(current_ptr));

            match self.get_next_file_ptr(current_ptr) {
                Some(ptr) => current_ptr = ptr,
                None => break,
            }
        }
    }

    pub fn print_file_content(&self, filename: &str) {
        let mut current_ptr = self.header_ptr;
        loop {
            let name = self.get_file_name(current_ptr).trim_end_matches('\0');
            if name == filename {
                let file_content = self.get_file_content(current_ptr);
                println(core::str::from_utf8(file_content).unwrap());
                return;
            }

            match self.get_next_file_ptr(current_ptr) {
                Some(ptr) => current_ptr = ptr,
                None => break,
            }
        }
        println("File not found");
    }

    pub fn load_file_to_memory(&self, filename: &str, addr: *mut u8) -> bool {
        match self.get_file_content_by_name(filename) {
            Some(content) => {
                unsafe {
                    core::ptr::copy_nonoverlapping(content.as_ptr(), addr, content.len());
                }
                true
            }
            None => false,
        }
    }

    pub fn get_num_files(&self) -> u32 {
        let mut num_files = 0;
        let mut current_ptr = self.header_ptr;
        loop {
            num_files += 1;
            match self.get_next_file_ptr(current_ptr) {
                Some(ptr) => current_ptr = ptr,
                None => break,
            }
        }
        num_files
    }

    fn get_next_file_ptr(&self, ptr: *const u8) -> Option<*const u8> {
        let namesize = self.get_namesize(ptr);
        let filesize = self.get_filesize(ptr);

        let mut offset = 110 + namesize;
        // if offset % 2 != 0 {
        //     offset += 1;
        // }
        if offset % 4 != 0 {
            offset += 4 - (offset % 4);
        }
        if filesize > 0 {
            offset += filesize;

            if offset % 4 != 0 {
                offset += 4 - (offset % 4);
            }
        }

        unsafe {
            let next_ptr = ptr.add(offset as usize);

            // Check if the ptr is pointed to the magic number
            assert_eq!(*next_ptr, b'0');

            if self.get_namesize(next_ptr) == 11
                && self.get_file_name(next_ptr).starts_with("TRAILER!!!")
            {
                return None;
            }

            Some(next_ptr)
        }
    }

    fn get_file_content_by_name(&self, name: &str) -> Option<&[u8]> {
        let mut current_ptr = self.header_ptr;
        loop {
            let file_name = self.get_file_name(current_ptr).trim_end_matches('\0');
            if file_name == name {
                return Some(self.get_file_content(current_ptr));
            }
            match self.get_next_file_ptr(current_ptr) {
                Some(ptr) => current_ptr = ptr,
                None => break,
            }
        }

        None
    }

    fn get_file_content(&self, ptr: *const u8) -> &[u8] {
        let namesize = self.get_namesize(ptr);
        let filesize = self.get_filesize(ptr);

        if filesize == 0 {
            return &[];
        }

        let mut offset = 110 + namesize;
        if offset % 2 != 0 {
            offset += 1;
        }

        if offset % 4 != 0 {
            offset += 4 - (offset % 4);
        }

        let file_ptr = unsafe { ptr.offset(offset as isize) };

        unsafe { core::slice::from_raw_parts(file_ptr, filesize as usize) }
    }

    fn get_file_name(&self, ptr: *const u8) -> &str {
        let namesize = self.get_namesize(ptr);

        let bytes_slice =
            unsafe { core::slice::from_raw_parts(ptr.offset(110), namesize as usize) };

        core::str::from_utf8(bytes_slice).unwrap()
    }

    fn get_namesize(&self, ptr: *const u8) -> u32 {
        let namesize = self.get_header_ascii(ptr, CpioHeaderField::Namesize);
        CpioArchive::ascii_to_u32(namesize)
    }

    fn get_filesize(&self, ptr: *const u8) -> u32 {
        let filesize = self.get_header_ascii(ptr, CpioHeaderField::Filesize);
        CpioArchive::ascii_to_u32(filesize)
    }

    fn ascii_to_u32(hex: u64) -> u32 {
        // println("ascii_to_u32");
        let mut value: u32 = 0;
        for i in 0..8 {
            let shift = i;
            let char_value = ((hex >> (shift * 8)) & 0xFF) as u8;
            // print_hex(char_value as u32);

            let digit = if char_value >= b'0' && char_value <= b'9' {
                char_value - b'0'
            } else if char_value >= b'A' && char_value <= b'F' {
                char_value - b'A' + 10
            } else if char_value >= b'a' && char_value <= b'f' {
                char_value - b'a' + 10
            } else {
                0
            };
            value += (digit as u32) << (shift * 4);
            // print_hex(value as u32);
        }
        value
    }

    fn get_header_ascii(&self, ptr: *const u8, field: CpioHeaderField) -> u64 {
        let offset = match field {
            CpioHeaderField::Magic => 0,
            CpioHeaderField::Ino => 6,
            CpioHeaderField::Mode => 14,
            CpioHeaderField::Uid => 22,
            CpioHeaderField::Gid => 30,
            CpioHeaderField::Nlink => 38,
            CpioHeaderField::Mtime => 46,
            CpioHeaderField::Filesize => 54,
            CpioHeaderField::Devmajor => 62,
            CpioHeaderField::Devminor => 70,
            CpioHeaderField::Rdevmajor => 78,
            CpioHeaderField::Rdevminor => 86,
            CpioHeaderField::Namesize => 94,
            CpioHeaderField::Check => 102,
        };

        let len = match field {
            CpioHeaderField::Magic => 6,
            _ => 8,
        };

        let mut value: u64 = 0;
        unsafe {
            let ptr = ptr.offset(offset);
            for i in 0..len {
                value |= (*(ptr.offset(i as isize)) as u64) << ((len - i - 1) * 8);
            }
        }
        value
    }
}
