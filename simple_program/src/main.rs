#![no_main]
#![no_std]

use core::arch::{asm, global_asm};

mod cpu;
mod os;
extern crate alloc;

global_asm!(include_str!("boot.s"));

#[no_mangle]
fn main() {
    // let mut el: u64 = 0;
    unsafe {
        cpu::uart::initialize();
        // asm!("mrs {tmp}, CurrentEL", tmp = out(reg) el);
        // asm!("nop");
    }
    println!("Hello, world!");
    // println!("Current EL: {}", el >> 2);

    loop {}
}
