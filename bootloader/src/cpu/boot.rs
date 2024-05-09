use core::arch::{asm, global_asm};

use crate::stdio::println;
use crate::uart_load::load_kernel;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() {
    crate::cpu::uart::initialize();
    println("+------------------------------------+");
    println("+   Bootloader v1.2 by @zolark173    +");
    println("+------------------------------------+");
    println("Please send a kernel image...");

    load_kernel();

    println("Kernel loaded!");
    println("Jumping to kernel...");
    println("");

    // Wait for a while to make sure the UART output is sent
    for _ in 0..10000 {
        asm!("nop");
    }

    // Jump to the kernel
    asm!(
        "ldr x2, =0x75100",
        "ldr x0, [x2]",
        "ldr x1, =0x80000",
        "br x1"
    );
}
