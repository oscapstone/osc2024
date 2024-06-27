use core::panic::PanicInfo;
use alloc::string::ToString;
use crate::cpu::reboot;

use crate::os::stdio::*;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println_now("PANIC");
    println_now(info.to_string().as_str());
    reboot::reset(1000);
    loop {}
}
