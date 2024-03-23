use crate::cpu;
use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    cpu::wait_forever()
}
