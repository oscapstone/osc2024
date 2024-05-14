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
