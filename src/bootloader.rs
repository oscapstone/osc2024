#![no_std]

#[panic_handler]
pub extern "Rust" fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod mmio;
mod peripheral;
mod stdio;
mod utils;

use peripheral::uart;

const KERNEL_ADDR: u32 = 0x80000;

#[no_mangle]
pub fn main() {
    uart::init();
    // mmio::MMIO::delay(10000000);
    stdio::puts(b"Hello, world!");
    // stdio::puts(b"Please send a kernel image.");

    // loop {
    //     let buf = uart::recv();
    //     let c = utils::to_hex(buf as u32);
    //     stdio::puts(&c);
    // }
    // receive kernel until delay 1 second
    let mut pos = KERNEL_ADDR;
    let mut delay = 0;
    loop {
        if let Some(c) = uart::recv_nb() {
            unsafe {
                core::ptr::write_volatile(pos as *mut u8, c);
            }
            delay = 0;
            if pos % 1000 == 0 {
                stdio::write(b".");
            }
            pos += 1;
        } else {
            if pos == KERNEL_ADDR {
                continue;
            }
            delay += 1;
            if delay >= 10000 {
                if pos < KERNEL_ADDR + 1000 {
                    stdio::puts(b"Failed to get kernel image");
                    pos = KERNEL_ADDR;
                    delay = 0;
                    continue;
                }
                stdio::puts(b"Got kernel!");
                break;
            }
        }
        mmio::MMIO::delay(1000);
    }

    // jump to KERNEL_ADDR
    let kernel = KERNEL_ADDR as *const ();
    let kernel: fn() = unsafe { core::mem::transmute(kernel) };
    kernel();
    stdio::puts(b"Jumping to kernel");
}
