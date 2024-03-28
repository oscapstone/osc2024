#![no_std]

use core::{mem::align_of, slice, str};

use library::time::Time;
use vfs::file::FileMetadata;

const NEW_ASCII_CPIO_MAGIC: [u8; 6] = [b'0', b'7', b'0', b'7', b'0', b'1'];

#[derive(Debug, Clone)]
#[repr(packed)]
pub struct CPIOHeader {
    magic: [u8; 6],
    ino: [u8; 8],
    mode: [u8; 8],
    uid: [u8; 8],
    gid: [u8; 8],
    nlink: [u8; 8],
    mtime: [u8; 8],
    filesize: [u8; 8],
    devmajor: [u8; 8],
    devminor: [u8; 8],
    rdevmajor: [u8; 8],
    rdevminor: [u8; 8],
    namesize: [u8; 8],
    check: [u8; 8],
}

impl CPIOHeader {
    fn mode(&self) -> u32 {
        Self::parse_field(&self.mode)
    }

    fn uid(&self) -> u32 {
        Self::parse_field(&self.uid)
    }

    fn gid(&self) -> u32 {
        Self::parse_field(&self.gid)
    }

    fn mtime(&self) -> u32 {
        Self::parse_field(&self.mtime)
    }

    fn namesize(&self) -> u32 {
        Self::parse_field(&self.namesize)
    }

    fn filesize(&self) -> u32 {
        Self::parse_field(&self.filesize)
    }

    #[inline(always)]
    fn parse_field(field: &[u8; 8]) -> u32 {
        let s = str::from_utf8(field).unwrap();
        u32::from_str_radix(s, 16).unwrap()
    }
}

#[derive(Debug, Clone)]
pub struct File<'a> {
    pub name: &'a str,
    pub content: &'a [u8],
    pub metadata: FileMetadata,
}

pub struct CPIOArchive {
    current: usize,
}

impl CPIOArchive {
    pub const unsafe fn from_memory(mmio_start_addr: usize) -> Self {
        Self {
            current: mmio_start_addr,
        }
    }

    pub fn read_next(&mut self) -> Option<File> {
        let cpio_header_addr = self.current as *const CPIOHeader;
        let cpio_header = unsafe { &*cpio_header_addr };
        // check magic number
        if cpio_header.magic != NEW_ASCII_CPIO_MAGIC {
            panic!("Provide mmio start address is not a CPIO archive");
        }
        let name_start_addr = unsafe { cpio_header_addr.add(1) as *const u8 };
        let namesize = cpio_header.namesize();
        let file_name = unsafe {
            str::from_utf8(slice::from_raw_parts(name_start_addr, namesize as usize)).unwrap()
        };
        if file_name == "TRAILER!!!\0" {
            return None;
        }
        // the address should align 32 bits
        let content_start_addr_without_align = unsafe { name_start_addr.add(namesize as usize) };
        let content_start_addr = unsafe {
            content_start_addr_without_align
                .add(content_start_addr_without_align.align_offset(align_of::<u32>()))
        };
        let filesize = cpio_header.filesize();
        let file_content =
            unsafe { slice::from_raw_parts(content_start_addr as *const u8, filesize as usize) };
        let mtime = Time::new(cpio_header.mtime() as i64, 0);
        // the address should align 32 bits
        let new_current_without_align = unsafe { content_start_addr.add(filesize as usize) };
        self.current = unsafe {
            new_current_without_align.add(new_current_without_align.align_offset(align_of::<u32>()))
                as usize
        };
        Some(File {
            name: file_name,
            content: file_content,
            metadata: FileMetadata {
                umode: cpio_header.mode() as u16,
                uid: cpio_header.uid(),
                gid: cpio_header.gid(),
                // use mtime instead
                atime: mtime,
                mtime: mtime,
                // use mtime instead
                ctime: mtime,
            },
        })
    }
}
