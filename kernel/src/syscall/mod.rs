use crate::scheduler;
pub fn get_pid() -> u64 {
    let pid = scheduler::get().current.unwrap() as u64;
    pid
}

pub fn write(buf: *const u8, size: usize) -> usize {
    let mut written = 0;
    for i in 0..size {
        unsafe {
            let c = *buf.add(i);
            driver::uart::send(c);
        }
        written += 1;
    }
    written
}
