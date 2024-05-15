use core::arch::asm;

pub fn get_pid() -> u64 {
    let pid: u64;
    unsafe {
        asm!(
            "svc 0",
            in("x8") 0,
            out("x0") pid,
        );
    }
    pid
}

pub fn read(buf: *const u8, size: usize) -> usize {
    let read: usize;
    unsafe {
        asm!(
            "svc 0",
            inout("x0") buf => read,
            in("x1") size,
            in("x8") 1,
        );
    }
    read
}

pub fn write(buf: *const u8, size: usize) -> usize {
    let written: usize;
    unsafe {
        asm!(
            "svc 0",
            inout("x0") buf => written,
            in("x1") size,
            in("x8") 2,
        );
    }
    written
}
