use crate::{file_system::virtual_file_system, scheduler::current};

pub fn read(fd: i32, buf: *mut u8, count: usize) -> i32 {
    let current = unsafe { &mut *current() };
    let file = match current.get_file(fd as usize) {
        Ok(file_descriptor) => virtual_file_system()
            .get_file(file_descriptor.file_handle_index())
            .unwrap(),
        Err(_) => return -1,
    };
    let buf = unsafe { core::slice::from_raw_parts_mut(buf, count) };
    match file.read(buf, count) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}
