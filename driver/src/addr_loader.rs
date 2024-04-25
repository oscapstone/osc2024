
pub fn load_dtb_addr() -> *mut u8 {
    let dtb_addr: *mut u64;
    unsafe {
        core::arch::asm!("ldr {}, =__dtb_addr", out(reg) dtb_addr);
        (*dtb_addr) as *mut u8
    }
}

pub fn usr_load_prog_base() -> *mut u8 {
    let addr: *mut u64;
    unsafe {
        core::arch::asm!("ldr {}, =__usr_prog_start", out(reg) addr);
        addr as *mut u8
    }
}
