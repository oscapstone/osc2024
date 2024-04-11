#![cfg(not(test))]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        stdio::println!("Kernel panic!");
    }
}
