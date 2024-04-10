use core::arch::global_asm;

use stdio::println;

global_asm!(include_str!("kernel.s"));

#[no_mangle]
extern "C" fn _start_rust() {
    println!("Kernel starting main...");
    crate::main();
}
