#![no_std]
#![no_main]
#![feature(allocator_api)]
#![feature(btreemap_alloc)]

extern crate alloc;

mod allocator;
mod commands;
mod dtb;
mod exception;
mod kernel;
mod panic;
mod timer;

use alloc::boxed::Box;
use allocator::buddy::BUDDY_SYSTEM;
#[allow(unused_imports)]
use allocator::utils::{toggle_buddy_verbose, toggle_bump_verbose, toggle_dynamic_verbose};
use core::arch::asm;
use core::time::Duration;
use stdio::{debug, gets, print, println};

pub static mut INITRAMFS_ADDR: u32 = 0;
const MAX_COMMAND_LEN: usize = 0x100;

fn main() -> ! {
    println!("Hello, world!");
    print_mailbox_info();
    debug!("Dealing with dtb...");
    unsafe {
        INITRAMFS_ADDR = dtb::get_initrd_start().unwrap();
    }
    debug!("Initramfs address: {:#x}", unsafe { INITRAMFS_ADDR });
    unsafe {
        BUDDY_SYSTEM.init();
    }
    unsafe {
        // BUDDY_SYSTEM.toggle_verbose();
        BUDDY_SYSTEM.reserve_by_addr_range(0x0000, 0x1000);
        BUDDY_SYSTEM.reserve_by_addr_range(0x0_0000, 0x8_0000); // kernel stack reserved
        BUDDY_SYSTEM.reserve_by_addr_range(0x8_0000, 0x10_0000); // kernel code reserved
        BUDDY_SYSTEM.reserve_by_addr_range(0x800_0000, 0x820_0000); // qemu initramfs reserved
        BUDDY_SYSTEM.reserve_by_addr_range(0x820_0000, 0x880_0000); // qemu dtb reserved
        BUDDY_SYSTEM.reserve_by_addr_range(0x1000_0000, 0x1100_0000); // bump allocator reserved
        BUDDY_SYSTEM.reserve_by_addr_range(0x2eff_0000, 0x2fff_0000); // rpi3 dtb reserved
        BUDDY_SYSTEM.print_info();
        // toggle_buddy_verbose();
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
    // toggle_dynamic_verbose();
    // toggle_bump_verbose();
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
    } else if command.starts_with(b"buddy") {
        commands::buddy::exec();
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
    println!("ARM memory: {:x} - {:x}", lb, ub);
}
