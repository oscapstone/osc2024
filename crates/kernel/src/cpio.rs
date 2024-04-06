#![no_std]

use core::marker::PhantomData;

const CPIO_NEWC_MAGIC: &[u8] = b"070701";
const CPIO_NEWC_END_NAME: &str = "TRAILER!!!";

#[allow(dead_code)]
#[repr(packed)]
#[derive(Debug)]
struct CpioNewcHeader {
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

macro_rules! hex_to_usize {
    ($src:expr) => {
        usize::from_str_radix(core::str::from_utf8($src).unwrap(), 16).unwrap()
    };
}

impl CpioNewcHeader {
    fn is_valid(&self) -> bool {
        &self.magic == CPIO_NEWC_MAGIC
    }

    fn namesize(&self) -> usize {
        hex_to_usize!(&self.namesize)
    }

    fn filesize(&self) -> usize {
        hex_to_usize!(&self.filesize)
    }
}

#[derive(Debug)]
pub struct CpioEntry<'a> {
    pub filename: &'a str,
    pub content: &'a [u8],
}

#[derive(Debug)]
pub struct CpioArchive {
    addr: usize,
}

pub struct Iter<'a> {
    current_addr: usize,
    marker: PhantomData<CpioEntry<'a>>,
}

impl<'a> Iterator for Iter<'a> {
    type Item = CpioEntry<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        let header_addr = self.current_addr as *const CpioNewcHeader;
        let header = unsafe { &*header_addr };
        if !header.is_valid() {
            return None;
        }

        let name_addr = unsafe { header_addr.add(1) as *const u8 };
        let namesize = header.namesize();
        let filename = unsafe {
            // the name is null-terminated so we should use namesize - 1
            let name_slice = core::slice::from_raw_parts(name_addr, namesize - 1);
            core::str::from_utf8_unchecked(name_slice)
        };
        if filename == CPIO_NEWC_END_NAME {
            return None;
        }

        let content_addr = unsafe {
            let addr = name_addr.add(namesize);
            // name is padded to 4 bytes
            addr.add(addr.align_offset(4))
        };
        let filesize = header.filesize();
        let content = unsafe { core::slice::from_raw_parts(content_addr, filesize) };

        self.current_addr = unsafe {
            let next_addr = content_addr.add(filesize);
            // align to 4 bytes
            next_addr.add(next_addr.align_offset(4)) as usize
        };

        Some(CpioEntry { filename, content })
    }
}

impl CpioArchive {
    /// Create a new CpioArchive from a given address.
    /// Safety: The address must be valid and point to a valid cpio archive.
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            addr: mmio_start_addr,
        }
    }

    pub fn files(&self) -> Iter {
        Iter {
            current_addr: self.addr,
            marker: PhantomData,
        }
    }
}
