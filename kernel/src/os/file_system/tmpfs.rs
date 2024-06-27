use super::vfs::{FileHandle, FileSystem};
use crate::alloc::string::ToString;
use alloc::boxed::Box;
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;

struct TmpFS {
    files: Vec<(String, Vec<u8>)>,
}

pub fn init() -> Box<dyn FileSystem> {
    let mut ret = Box::new(TmpFS { files: Vec::new() });
    ret.create("/");
    ret.create("/dev/");
    ret.create("/dev/null");
    ret.create("/test.txt");
    ret.write("/test.txt", 0, b"Hello, world!", 13);
    ret
}

impl FileSystem for TmpFS {
    fn get_fs_name(&self) -> String {
        String::from("tmpfs")
    }

    fn create(&mut self, path: &str) {
        self.files.push((path.to_string(), Vec::new()));
    }

    fn open(&mut self, path: &str) -> bool {
        self.files.iter().any(|(name, _)| name == path)
    }

    fn close(&mut self) {
        // do nothing
    }

    fn lookup(&self, path: &str) -> bool {
        self.files.iter().any(|(n, _)| *n == path)
    }

    fn read(&self, path: &str, offset: usize, buf: &mut [u8], len: usize) -> usize {
        if let Some((_, data)) = self.files.iter().find(|(name, _)| name == path) {
            let data = &data[offset..];
            let len = core::cmp::min(len, data.len());
            buf[..len].copy_from_slice(&data[..len]);
            len
        } else {
            0
        }
    }

    fn write(&mut self, path: &str, offset: usize, buf: &[u8], len: usize) -> usize {
        if let Some((_, data)) = self.files.iter_mut().find(|(name, _)| name == path) {
            if offset >= data.len() {
                data.resize(offset + len, 0);
            }
            for i in 0..len {
                data[offset + i] = buf[i];
            }
            len
        } else {
            0
        }
    }
}
