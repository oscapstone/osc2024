#![no_main]
#![no_std]

mod os;
use core::{
    arch::{asm, global_asm},
    ptr::addr_of_mut,
};
use os::{
    stdio::{print, print_hex, println},
    system_call::{fork, get_pid, uart_read, uart_write},
};

use crate::os::system_call;

// extern crate alloc;

#[no_mangle]
unsafe fn main() {
    loop {
        println("Hello World!");
        print_hex(get_pid() as u64);
        let a = fork();
        if a == 0 {
            println("Child");
        } else {
            println("Parent");
            loop {}
        }
        println("NOPING");
        for _ in 0..1000000000 {
            asm!("nop");
        }
    }
    loop {}
}
