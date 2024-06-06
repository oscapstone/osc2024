use crate::scheduler::current;

pub fn close(fd: i32) -> i32 {
    let current = unsafe { &mut *current() };
    match current.close_file(fd as usize) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}
