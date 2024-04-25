use crate::{cpu};
use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    crate::println!("Kernel panic: {}", _info);
    cpu::wait_forever()
}
