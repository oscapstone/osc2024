use core::arch::asm;

pub fn get_pid() -> u64 {
    let pid: u64;
    unsafe {
        asm!(
            "mov x8, 0",
            "svc 0",
            out("x8") pid,
        );
    }
    pid
}
