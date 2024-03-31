#![no_std]
#![no_main]

mod boot;
mod panic;

use driver::uart;
use stdio::{print, print_u32, println};

const KERNEL_ADDR: u32 = 0x80000;

fn main() {
    uart::init();
    println("Hello, world!");

    let dtb_addr = 0x8f000 as *const u8;
    let dtb_addr = unsafe { core::ptr::read_volatile(dtb_addr as *const u32) };
    print("DTB address: ");
    print_u32(dtb_addr);
    println("");

    println("Please send a kernel image.");
    // receive kernel until delay
    let mut pos = KERNEL_ADDR;
    let mut delay = 0;

    loop {
        if let Some(c) = uart::recv_nb() {
            unsafe {
                core::ptr::write_volatile(pos as *mut u8, c);
            }
            delay = 0;
            if pos % 0x400 == 0 {
                print(".");
            }
            pos += 1;
        } else {
            if pos == KERNEL_ADDR {
                continue;
            }
            delay += 1;
            if delay >= 1000000 {
                if pos < KERNEL_ADDR + 0x1000 {
                    println("");
                    println("Kernel image not received!");
                    println("Please send a kernel image.");
                    pos = KERNEL_ADDR;
                    delay = 0;
                    continue;
                }
                println("");
                println("Kernel image received!");
                print("Kernel size: ");
                print_u32(pos - KERNEL_ADDR);
                println(" bytes");
                break;
            }
        }
    }

    println("Jumping to kernel");
    // jump to KERNEL_ADDR
    let kernel = KERNEL_ADDR as *const ();
    let kernel: fn() = unsafe { core::mem::transmute(kernel) };
    kernel();
}
