use crate::exception::enable_interrupt;
use core::arch::global_asm;
use driver::uart::init;
use stdio::println;

global_asm!(include_str!("kernel.S"));

#[no_mangle]
extern "C" fn _start_rust() {
    init();
    unsafe {
        enable_interrupt();
    }
    println!("Kernel starting main...");
    crate::main();
}
