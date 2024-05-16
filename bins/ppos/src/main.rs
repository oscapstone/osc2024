#![no_std]
#![no_main]

extern crate alloc;

mod driver;
mod exception;
mod memory;
mod pid;
mod scheduler;
mod shell;
mod signal;
mod system_call;

use alloc::rc::Rc;
use core::{arch::global_asm, panic::PanicInfo};
use cpu::cpu::{enable_kernel_space_interrupt, switch_to_el1};
use library::{
    console::{Console, ConsoleMode},
    println,
    sync::mutex::Mutex,
};
use pid::pid_manager;
use scheduler::{round_robin_scheduler::ROUND_ROBIN_SCHEDULER, task::Task};
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
    memory::init_allocator();
    exception::handling_init();
    driver::init().unwrap();
    pid_manager().init();
    scheduler::register_scheduler(&ROUND_ROBIN_SCHEDULER);
    mini_uart().change_mode(ConsoleMode::Async);
    enable_kernel_space_interrupt();
    kernel_start();
}

fn kernel_start() -> ! {
    scheduler::scheduler().add_task(Rc::new(Mutex::new(Task::from_job(run_shell))));
    scheduler::scheduler().start_scheduler();
}

pub fn run_shell() -> ! {
    let mut shell = Shell::new();
    shell.run();
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    mini_uart().change_mode(ConsoleMode::Sync);
    println!("{}", _info);
    loop {}
}
