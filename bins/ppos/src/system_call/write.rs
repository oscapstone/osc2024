use core::slice;

use crate::{file_system::virtual_file_system, scheduler::current};

pub fn write(fd: i32, buf: *const u8, count: usize) -> i32 {
    let current = unsafe { &mut *current() };
    let file = match current.get_file(fd as usize) {
        Ok(file_descriptor) => virtual_file_system()
            .get_file(file_descriptor.file_handle_index())
            .unwrap(),
        Err(_) => return -1,
    };
    match file.write(unsafe { slice::from_raw_parts(buf, count) }, count) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}
