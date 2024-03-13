//! Set the pinicking behavior to wait infinitely.

#![no_std]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    cpu::wait_forever();
}
