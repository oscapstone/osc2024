#![no_std]

#[panic_handler]
pub extern "Rust" fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod mmio;
mod stdio;
mod uart;
mod utils;

const MAX_COMMAND_LEN: usize = 0x400;

use core::arch::asm;

#[no_mangle]
pub fn main() {
    uart::uart_init();
    // mmio::MMIO::delay(10000000);
    stdio::puts(b"Hello, world!");
    stdio::puts(b"Please send a kernel image..");

    // receive kernel until delay 1 second
    let mut pos = 0x80000;
    let mut delay = 0;
    loop {
        if pos == 0x80000 {
            delay = 0;
        }
        if delay >= 10000 {
            stdio::puts(b"Got kernel!");
            break;
        }
        if let Some(c) = uart::recv_nb() {
            unsafe {
                core::ptr::write_volatile(pos as *mut u8, c);
            }
            pos += 1;
        } else {
            delay += 1;
        }
        mmio::MMIO::delay(1000);
    }

    // jump to 0x80000
    let kernel = 0x80000 as *const ();
    let kernel: fn() = unsafe { core::mem::transmute(kernel) };
    kernel();
    stdio::puts(b"Jumping to kernel");
}
