#![no_std]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {
        stdio::puts(b"Panic!");
    }
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
    stdio::puts(b"Please send a kernel image.");

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
                stdio::write(b".");
            }
            pos += 1;
        } else {
            if pos == KERNEL_ADDR {
                continue;
            }
            delay += 1;
            if delay >= 1000000 {
                stdio::println("");
                stdio::println("Kernel image received!");
                stdio::print("Kernel size: ");
                stdio::print_u32(pos - KERNEL_ADDR);
                stdio::println(" bytes");
                break;
            }
        }
    }

    mmio::MMIO::delay(1000000);
    stdio::puts(b"Jumping to kernel");
    // jump to KERNEL_ADDR
    let kernel = KERNEL_ADDR as *const ();
    let kernel: fn() = unsafe { core::mem::transmute(kernel) };
    kernel();
}
