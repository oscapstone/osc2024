use core::arch::{asm, global_asm};

use stdio::println;

use driver::uart::init;

global_asm!(include_str!("kernel.S"));

#[no_mangle]
extern "C" fn _start_rust() {
    init();
    unsafe {
        asm!("msr DAIFClr, 0xf");
    }
    println!("Kernel starting main...");
    crate::main();
}
