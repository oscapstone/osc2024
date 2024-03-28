#![no_std]
#![no_main]

extern crate alloc;

mod driver;
mod memory;
mod shell;

use core::{arch::global_asm, panic::PanicInfo};
use cpu::{
    cpu::switch_to_el1,
    exception::{self},
};
use library::println;
use shell::Shell;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe extern "C" fn _start_rust(devicetree_start_addr: usize) -> ! {
    memory::DEVICETREE_START_ADDR = devicetree_start_addr;
    // switch to EL1 and jump to kernel_init
    switch_to_el1(
        &memory::__phys_binary_load_addr as *const usize as u64,
        kernel_init,
    );
}

unsafe fn kernel_init() -> ! {
    exception::handling_init();
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
