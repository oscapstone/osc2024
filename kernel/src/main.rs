#![no_std]
#![no_main]

extern crate alloc;

mod allocator;
mod dtb;
mod exception;
mod kernel;
mod panic;
mod timer;

use alloc::boxed::Box;
use core::arch::asm;
use core::time::Duration;
use driver::watchdog;
use stdio::{gets, print, println};

static mut INITRAMFS_ADDR: u32 = 0;
const MAX_COMMAND_LEN: usize = 0x400;
const PROGRAM_ENTRY: *const u8 = 0x30010000 as *const u8;

fn main() -> ! {
    for _ in 0..1000000 {
        unsafe {
            asm!("nop");
        }
    }
    // uart::init();
    println!("Hello, world!");
    print_mailbox_info();

    println!("Dealing with dtb...");
    unsafe {
        INITRAMFS_ADDR = dtb::get_initrd_start().unwrap();
    }
    println!("Initramfs address: {:#x}", unsafe { INITRAMFS_ADDR });

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
        println!("Hello, world!");
    } else if command.starts_with(b"help") {
        println!("hello\t: print this help menu");
        println!("help\t: print Hello World!");
        println!("reboot\t: reboot the Raspberry Pi");
        println!("setTimeout: set a timer to reboot the Raspberry Pi");
    } else if command.starts_with(b"reboot") {
        watchdog::reset(100);
    } else if command.starts_with(b"ls") {
        let rootfs = filesystem::cpio::CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
        rootfs.print_file_list();
    } else if command.starts_with(b"cat") {
        let rootfs = filesystem::cpio::CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
        let filename = &command[4..];
        if let Some(data) = rootfs.get_file(core::str::from_utf8(filename).unwrap()) {
            print!("{}", core::str::from_utf8(data).unwrap());
        } else {
            println!(
                "File not found: {}",
                core::str::from_utf8(filename).unwrap()
            );
        }
    } else if command.starts_with(b"exec") {
        let rootfs = filesystem::cpio::CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
        let filename = &command[5..];
        if let Some(data) = rootfs.get_file(core::str::from_utf8(filename).unwrap()) {
            unsafe {
                core::ptr::copy(data.as_ptr(), PROGRAM_ENTRY as *mut u8, data.len());
                asm!(
                    "mov {0}, 0x3c0",
                    "msr spsr_el1, {0}",
                    "msr elr_el1, {1}",
                    "msr sp_el0, {2}",
                    "eret",
                    out(reg) _,
                    in(reg) PROGRAM_ENTRY,
                    in(reg) PROGRAM_ENTRY as u64 - 0x1000,
                );
            }
        } else {
            println!(
                "File not found: {}",
                core::str::from_utf8(filename).unwrap()
            );
        }
    } else if command.starts_with(b"echo") {
        for c in &command[5..] {
            driver::uart::send_async(*c);
        }
        println!("Echoed: {}", core::str::from_utf8(&command[5..]).unwrap());
        loop {
            if let Some(c) = driver::uart::recv_async() {
                println!("Received: {} (0x{:x})", c as char, c);
                if c == b'\n' {
                    break;
                }
            }
        }
    } else if command.starts_with(b"test") {
        let tm = timer::manager::get_timer_manager();
        tm.add_timer(
            Duration::from_secs(1),
            Box::new(|| {
                println!("This is the first timmer.");
            }),
        );

        let msg = "Hello, world!";
        tm.add_timer(
            Duration::from_secs(6),
            Box::new(move || {
                println!("{}", msg);
            }),
        );
    } else {
        println!(
            "Unknown command: {}",
            core::str::from_utf8(command).unwrap()
        );
    }
}

#[inline(never)]
fn print_mailbox_info() {
    let revision = driver::mailbox::get_board_revision();
    let (lb, ub) = driver::mailbox::get_arm_memory();
    println!("Board revision: {:x}", revision);
    println!("ARM memory: {:x} - {:x}", lb, ub);
}
