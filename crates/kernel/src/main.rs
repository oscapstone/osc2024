#![no_std]
#![no_main]

use core::arch::global_asm;
use panic_wait as _;
use small_std::println;

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn main() -> ! {
    use small_std::fmt::print::console::console;

    println!("[0] Hello from Rust!");

    println!("[1] Chars written: {}", console().chars_written());

    println!("[2] Stopping here.");
    cpu::wait_forever();
}
