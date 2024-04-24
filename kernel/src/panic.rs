#![cfg(not(test))]

use core::arch::asm;
use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        stdio::println!("Kernel panic!");
        stdio::println!("{:?}", _info);
        unsafe { asm!("b .") }
    }
}
