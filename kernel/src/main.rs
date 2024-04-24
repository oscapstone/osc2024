#![no_std]
#![no_main]

extern crate alloc;

mod allocator;
mod commands;
mod dtb;
mod exception;
mod kernel;
mod panic;
mod timer;

use alloc::boxed::Box;
use core::arch::asm;
use core::time::Duration;
use stdio::{debug, gets, print, println};

pub static mut INITRAMFS_ADDR: u32 = 0;
const MAX_COMMAND_LEN: usize = 0x100;

fn main() -> ! {
    for _ in 0..1000000 {
        unsafe {
            asm!("nop");
        }
    }
    // uart::init();
    println!("Hello, world!");
    print_mailbox_info();
    debug!("Dealing with dtb...");
    unsafe {
        INITRAMFS_ADDR = dtb::get_initrd_start().unwrap();
    }
    debug!("Initramfs address: {:#x}", unsafe { INITRAMFS_ADDR });
    // Get current timer value
    unsafe {
        let mut freq: u64;
        let mut now: u64;
        asm!(
            "mrs {freq}, cntfrq_el0",
            "mrs {now}, cntpct_el0",
            freq = out(reg) freq,
            now = out(reg) now,
        );
        println!("Current time: {}", now);
        println!("Frequency: {} Hz", freq);
        println!("Boot time: {} ms", now / (freq / 1000));
    }

    timer::manager::init_timer_manager();

    unsafe {
        exception::enable_inturrupt();
    }
    {
        let tm = timer::manager::get_timer_manager();

        tm.add_timer(
            Duration::from_secs(2),
            Box::new(|| {
                println!("First boot timer expired!");
            }),
        );
    }

    let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
    loop {
        print!("> ");
        gets(&mut buf);
        execute_command(&buf);
    }
}

#[inline(never)]
fn execute_command(command: &[u8]) {
    if command.starts_with(b"\x00") {
        return;
    } else if command.starts_with(b"hello") {
        commands::hello::exec();
    } else if command.starts_with(b"help") {
        commands::help::exec();
    } else if command.starts_with(b"reboot") {
        commands::reboot::exec();
    } else if command.starts_with(b"ls") {
        commands::ls::exec();
    } else if command.starts_with(b"cat") {
        commands::cat::exec(&command);
    } else if command.starts_with(b"exec") {
        commands::exec::exec(&command);
    } else if command.starts_with(b"echo") {
        commands::echo::exec(&command);
    } else if command.starts_with(b"setTimeOut") {
        commands::set_time_out::exec(&command);
    } else {
        println!(
            "Unknown command: {}",
            core::str::from_utf8(command).unwrap()
        );
    }
}

#[inline(never)]
fn print_mailbox_info() {
    println!("Printing mailbox info...");
    let revision = driver::mailbox::get_board_revision();
    println!("Board revision: {:x}", revision);
    let (lb, ub) = driver::mailbox::get_arm_memory();
    println!("Board revision: {:x}", revision);
    println!("ARM memory: {:x} - {:x}", lb, ub);
}
