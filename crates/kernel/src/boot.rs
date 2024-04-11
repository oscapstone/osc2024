use core::arch::{asm, global_asm};

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

pub static mut DEVICETREE_START_ADDR: usize = 0;

global_asm!(
    include_str!( "boot.s"),
    CONST_CORE_ID_MASK = const 0b11
);

#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    asm!("mov {}, x0", out(reg) DEVICETREE_START_ADDR);

    crate::kernel_init()
}
