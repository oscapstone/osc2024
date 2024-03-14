use core::arch::global_asm;

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    crate::kernel_init()
}
