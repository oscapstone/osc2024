//! TODO: The crate level documentation goes here.

#![no_main]
#![no_std]

mod bsp;
mod kernel;

/// Load architecture specific boot code
#[cfg(target_arch = "aarch64")]
#[path = "_arch/aarch64/boot.rs"]
mod boot;

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
