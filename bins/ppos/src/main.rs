#![no_std]
#![no_main]

extern crate alloc;

mod driver;
mod memory;
mod shell;

use core::{
    arch::{asm, global_asm},
    panic::PanicInfo,
};
use library::println;
use shell::Shell;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    asm!("mov {}, x0", out(reg) memory::DEVICETREE_START_ADDR);
    kernel_init();
}

unsafe fn kernel_init() -> ! {
    driver::init().unwrap();
    kernel_start();
}

fn kernel_start() -> ! {
    let mut shell = Shell::new();
    shell.run();
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("{}", _info);
    loop {}
}
