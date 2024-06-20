use core::arch::{asm, global_asm};
use driver::uart::init;
use stdio::println;

global_asm!(include_str!("kernel.S"));

#[no_mangle]
extern "C" fn _start_rust() {
    init();
    println!("Kernel starting main...");
    let sp: u64;
    unsafe {
        asm!("mov {}, sp", out(reg) sp);
    }
    println!("Stack pointer: {:#x}", sp);
    crate::main();
}
