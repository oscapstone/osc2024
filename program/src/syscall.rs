use core::arch::asm;

#[allow(dead_code)]
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

#[allow(dead_code)]
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

#[allow(dead_code)]
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

#[allow(dead_code)]
pub fn fork() -> u64 {
    let pid: u64;
    unsafe {
        asm!(
            "svc 0",
            out("x0") pid,
            in("x8") 4,
        );
    }
    pid
}

#[allow(dead_code)]
pub fn exit(status: u64) {
    unsafe {
        asm!(
            "svc 0",
            in("x0") status,
            in("x8") 5,
        );
    }
}

#[allow(dead_code)]
pub fn mbox_call(channel: u8, mbox: *mut u32) -> u64 {
    let ret: u64;
    unsafe {
        asm!(
            "svc 0",
            inout("x0") channel as u64 => ret,
            in("x1") mbox,
            in("x8") 6,
        );
    }
    ret
}
