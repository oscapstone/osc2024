#![no_main]
#![no_std]

use core::arch::global_asm;
use core::panic::PanicInfo;

global_asm!(include_str!("boot.S"));
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    unimplemented!()
}
