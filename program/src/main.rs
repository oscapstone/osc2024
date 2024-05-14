#![no_std]
#![feature(start)]

mod panic;
mod stdio;
mod syscall;

use stdio::{print, print_dec, print_hex, println};

#[start]
fn main(_: isize, _: *const *const u8) -> isize {
    println("[program] Hello, world!");
    for i in 0..1000 {
        let pid = syscall::get_pid();
        print("[program] PID=");
        print_hex(pid);
        print(", i=");
        print_dec(i);
        println("");
    }
    return 0;
}
