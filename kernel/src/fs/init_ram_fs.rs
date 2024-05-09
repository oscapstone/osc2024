/*
https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
https://github.com/rcore-os/cpio/blob/main/src/lib.rs
*/
extern crate alloc;

use crate::println;

const CPIO_END: &str = "TRAILER!!!\0";
const MAGIC_NUMBER: &[u8] = b"070701";
const HEADER_LEN: usize = 110;

pub(crate) struct Cpio {
    data: *const u8,
}

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

impl Cpio {
    pub fn load(data: *const u8) -> Cpio {
        Cpio { data }
    }

    pub fn print_file_list(&self) {
        let mut address = self.data;
        loop {
            let header = unsafe { &*(address as *const CpioNewcHeader) };
            if header.c_magic != *MAGIC_NUMBER {
                println!("[InitRamFs] Invalid CPIO magic");
                break;
            }
            let namesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.c_namesize) },
                16,
            )
            .unwrap();
            let filesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.c_filesize) },
                16,
            )
            .unwrap();
            let name = unsafe {
                core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                    address.add(HEADER_LEN),
                    namesize as usize,
                ))
            };

            if name == CPIO_END {
                break;
            }

            println!("{} {}", filesize, name);

            address = unsafe { address.add(pad_to_4(HEADER_LEN + namesize as usize)) };
            address = unsafe { address.add(pad_to_4(filesize as usize)) };
        }
    }

    pub fn get_file(&self, filename: &str) -> Option<&[u8]> {
        let mut address = self.data;
        loop {
            let header = unsafe { &*(address as *const CpioNewcHeader) };
            if header.c_magic != *MAGIC_NUMBER {
                break;
            }
            let namesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.c_namesize) },
                16,
            )
            .unwrap();
            let filesize = u64::from_str_radix(
                unsafe { core::str::from_utf8_unchecked(&header.c_filesize) },
                16,
            )
            .unwrap();
            let name = unsafe {
                core::str::from_utf8_unchecked(core::slice::from_raw_parts(
                    address.add(HEADER_LEN),
                    namesize as usize,
                ))
            };

            if name == CPIO_END {
                break;
            }

            address = unsafe { address.add(pad_to_4(HEADER_LEN + namesize as usize)) };

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
                return Some(unsafe { core::slice::from_raw_parts(address, filesize as usize) });
            }

            address = unsafe { address.add(pad_to_4(filesize as usize)) };
        }
        None
    }
}

fn pad_to_4(len: usize) -> usize {
    match len % 4 {
        0 => len,
        x => len + (4 - x),
    }
}
