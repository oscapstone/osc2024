use core::arch::global_asm;

use stdio::println;

global_asm!(include_str!("boot.s"));

#[no_mangle]
extern "C" fn _start_rust() {
    crate::main();
    println("Bootloader finished");
}
