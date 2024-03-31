use core::option::Option::{self, None, Some};
use stdio;

pub struct CpioArchive {
    data: *const u8,
}

#[repr(C, packed)]
struct CpioHeader {
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

impl CpioArchive {
    pub fn load(data: *const u8) -> CpioArchive {
        CpioArchive { data }
    }

    pub fn print_file_list(&self) {
        let mut current = self.data;
        loop {
            let header = unsafe { &*(current as *const CpioHeader) };
            if header.magic != *b"070701" {
                stdio::puts(b"Invalid CPIO magic");
                break;
            }
            let namesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.namesize) },
                16,
            )
            .unwrap();
            let filesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.filesize) },
                16,
            )
            .unwrap();
            let name = unsafe {
                core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                    current.add(110),
                    namesize as usize,
                ))
            };

            if name == "TRAILER!!!\0" {
                break;
            }

            stdio::puts(name.as_bytes());

            current = unsafe { current.add(110 + namesize as usize) };
            if current as usize % 4 != 0 {
                current = unsafe { current.add(4 - (current as usize % 4)) };
            }

            current = unsafe { current.add(filesize as usize) };

            if current as usize % 4 != 0 {
                current = unsafe { current.add(4 - (current as usize % 4)) };
            }
        }
    }

    pub fn get_file(&self, filename: &str) -> Option<&[u8]> {
        let mut current = self.data;
        loop {
            let header = unsafe { &*(current as *const CpioHeader) };
            if header.magic != *b"070701" {
                break;
            }
            let namesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.namesize) },
                16,
            )
            .unwrap();
            let filesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.filesize) },
                16,
            )
            .unwrap();
            let name = unsafe {
                core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                    current.add(110),
                    namesize as usize,
                ))
            };

            if name == "TRAILER!!!\0" {
                break;
            }

            current = unsafe { current.add(110 + namesize as usize) };
            if current as usize % 4 != 0 {
                current = unsafe { current.add(4 - (current as usize % 4)) };
            }

            let mut found = true;
            for i in 0..namesize as usize {
                if name.as_bytes()[i] == 0 {
                    break;
                }
                if name.as_bytes()[i] != filename.as_bytes()[i] {
                    found = false;
                    break;
                }
            }

            if found {
                return Some(unsafe { core::slice::from_raw_parts(current, filesize as usize) });
            }

            current = unsafe { current.add(filesize as usize) };
            if current as usize % 4 != 0 {
                current = unsafe { current.add(4 - (current as usize % 4)) };
            }
        }
        None
    }
}
