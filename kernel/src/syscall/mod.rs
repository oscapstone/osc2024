use stdio::println;

use crate::scheduler;
pub fn get_pid() -> u64 {
    let pid = scheduler::get().current.unwrap() as u64;
    pid
}

pub fn read(buf: *mut u8, size: usize) -> usize {
    let mut read = 0;
    for i in 0..size {
        unsafe {
            let c = driver::uart::recv();
            *buf.add(i) = c;
        }
        read += 1;
    }
    read
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

pub fn exit(status: u64) {
    println!("exit: {}", status);
    crate::scheduler::get().exit(status);
}
