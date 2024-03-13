#![no_std]
#![no_main]

use core::arch::global_asm;
use panic_wait as _;

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn main() -> ! {
    panic!();
}
