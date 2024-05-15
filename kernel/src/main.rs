#![no_std]
#![no_main]
#![feature(allocator_api)]
#![feature(btreemap_alloc)]
#![feature(duration_constructors)]

extern crate alloc;

mod allocator;
mod commands;
mod dtb;
mod exception;
mod kernel;
mod panic;
mod scheduler;
mod syscall;
mod thread;
mod timer;

use allocator::buddy::BUDDY_SYSTEM;
use stdio::{debug, gets, print, println};

pub static mut INITRAMFS_ADDR: u32 = 0;

fn main() -> ! {
    boot();
    println!("Kernel booted successfully!");
    // commands::execute(b"exec syscall.img");
    kernel_shell();
}

fn kernel_shell() -> ! {
    const MAX_COMMAND_LEN: usize = 0x100;
    let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
    loop {
        print!("> ");
        let len = gets(&mut buf);
        assert_eq!(buf[len], 0);
        commands::execute(&buf);
    }
}

fn boot() {
    println!("Hello, world!");
    print_mailbox_info();
    initramfs_init();
    buddy_init();
    timer::manager::init();
    print_boot_time();
    scheduler::init();
}

fn print_mailbox_info() {
    println!("Printing mailbox info...");
    let revision = driver::mailbox::get_board_revision();
    println!("Board revision: {:x}", revision);
    let (lb, ub) = driver::mailbox::get_arm_memory();
    println!("ARM memory: {:x} - {:x}", lb, ub);
}

fn initramfs_init() {
    unsafe {
        INITRAMFS_ADDR = dtb::get_initrd_start();
    }
    debug!("Initramfs address: {:#x}", unsafe { INITRAMFS_ADDR });
}

fn buddy_init() {
    unsafe {
        BUDDY_SYSTEM.init();
    }
    buddy_reserve_memory();
    allocator::utils::toggle_bump_verbose();
    unsafe {
        BUDDY_SYSTEM.print_info();
    }
}

fn buddy_reserve_memory() {
    let rsv_mem = dtb::get_reserved_memory();
    for (addr, size) in rsv_mem {
        unsafe {
            BUDDY_SYSTEM.reserve_by_addr_range(addr, addr + size);
        }
    }

    unsafe {
        BUDDY_SYSTEM.reserve_by_addr_range(0x0_0000, 0x8_0000); //
        BUDDY_SYSTEM.reserve_by_addr_range(0x6_0000, 0x8_0000); // kernel stack reserved
        BUDDY_SYSTEM.reserve_by_addr_range(0x8_0000, 0x10_0000); // kernel code reserved
    }

    unsafe {
        // initramfs reserved
        BUDDY_SYSTEM.reserve_by_addr_range(INITRAMFS_ADDR, INITRAMFS_ADDR + 0x4_0000);

        // bump allocator reserved
        BUDDY_SYSTEM.reserve_by_addr_range(
            allocator::config::BUMP_START_ADDR,
            allocator::config::BUMP_END_ADDR,
        );

        // rpi3 dtb reserved
        BUDDY_SYSTEM
            .reserve_by_addr_range(dtb::get_dtb_addr().0, dtb::get_dtb_addr().0 + 0x10_0000);
    }
}

fn print_boot_time() {
    let tm = crate::timer::manager::get();
    let now = tm.get_current();
    let freq = tm.get_frequency();
    println!("Frequency: {} Hz", freq);
    println!("Current time: {}", now);
    println!("Boot time: {} ms", now / (freq / 1000));
}
