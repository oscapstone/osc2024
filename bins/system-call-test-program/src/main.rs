#![no_std]
#![no_main]

mod console;
mod fork_test;
mod print;
mod shell;
mod string;
mod system_call;

use shell::Shell;

use core::{arch::global_asm, panic::PanicInfo};

global_asm!(include_str!("entry.s"));

#[no_mangle]
pub extern "C" fn _start_rust() -> ! {
    let mut shell = Shell::new();
    shell.run();
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
