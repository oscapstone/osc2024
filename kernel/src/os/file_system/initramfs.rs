use crate::println;

use super::cpio;
use super::vfs::{FileHandle, FileSystem};
use alloc::boxed::Box;
use alloc::format;
use alloc::string::String;

use super::super::shell::INITRAMFS;

struct InitramFS;

pub fn init() -> Box<dyn FileSystem> {
    Box::new(InitramFS)
}

impl FileSystem for InitramFS {
    fn get_fs_name(&self) -> String {
        String::from("initramfs")
    }

    fn create(&mut self, _path: &str) {
        println!("Cannot create file in initramfs");
    }

    fn open(&mut self, path: &str) -> bool {
        let initramfs = unsafe { INITRAMFS.as_ref().unwrap() };
        let path = path.trim_start_matches('/');

        println!("initramfs open: {}", path);

        initramfs.get_file_list().iter().any(|file| *file == path)
    }

    fn close(&mut self) {
        // do nothing
    }

    fn lookup(&self, path: &str) -> bool {
        let initramfs = unsafe { INITRAMFS.as_ref().unwrap() };
        let path = path.trim_start_matches('/');

        initramfs
            .get_file_list()
            .iter()
            .any(|file| file.trim_end_matches("\0") == path)
    }

    fn read(&self, path: &str, offset: usize, buf: &mut [u8], len: usize) -> usize {
        let initramfs = unsafe { INITRAMFS.as_ref().unwrap() };
        let path = path.trim_start_matches('/');

        initramfs.load_file_to_memory(path, buf.as_mut_ptr(), offset, len)
    }

    fn write(&mut self, path: &str, offset: usize, buf: &[u8], len: usize) -> usize {
        println!("Cannot write to initramfs");
        0
    }
}
