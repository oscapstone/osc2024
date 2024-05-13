#![no_std]
#![feature(start)]

mod panic;
mod stdio;
mod syscall;

use core::arch::asm;
use stdio::{print, print_hex, println};

#[start]
fn main(_: isize, _: *const *const u8) -> isize {
    // loop {}
    // uart::init();
    // uart::send(b'b');
    println("[program] Hello, world!");
    let pid = syscall::get_pid();
    for i in 0..1000 {
        print("[program] PID=");
        print_hex(pid);
        print(", i=");
        print_hex(i);
        println("");
        for _ in 0..100000000 {
            unsafe {
                asm! {
                    "nop"
                }
            }
        }
    }
    return 0;
}
