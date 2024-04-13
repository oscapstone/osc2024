#![no_std]
#![no_main]

extern crate alloc;

mod driver;
mod exception;
mod memory;
mod shell;

use core::{arch::global_asm, panic::PanicInfo};
use cpu::cpu::{enable_kernel_space_interrupt, switch_to_el1};
use library::{
    console::{Console, ConsoleMode, Write},
    println,
};
use shell::Shell;

use crate::driver::mini_uart;

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
    memory::init_allocator();
    mini_uart().change_mode(ConsoleMode::Async);
    enable_kernel_space_interrupt();
    kernel_start();
}

fn kernel_start() -> ! {
    let mut shell = Shell::new();
    shell.run();
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    mini_uart().write_fmt(format_args!("{}", _info));
    loop {}
}
