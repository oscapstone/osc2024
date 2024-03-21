use core::arch::{asm, global_asm};

use crate::stdio::println;
use crate::uart_load::load_kernel;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() { 
    crate::cpu::uart::initialize();
    println("Hello, world!");
    println("Please send a kernel image...");

    load_kernel();

    {
        println("Kernel loaded!");
        println("Jumping to kernel...");
        println("");
    }
    for _ in 0..10000 {
        asm!("nop");
    }
    asm!("ldr x0, =0x80000");
    asm!("br x0");
}