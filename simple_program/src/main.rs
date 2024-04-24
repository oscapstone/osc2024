#![no_main]
#![no_std]

use core::arch::{asm, global_asm};

mod cpu;
mod os;
extern crate alloc;

global_asm!(include_str!("boot.s"));

#[no_mangle]
fn main() {

    loop {}
}
