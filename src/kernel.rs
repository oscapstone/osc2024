#![no_std]
#![no_builtins]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod allocator;
mod filesystem;
mod mmio;
mod peripheral;
mod stdio;
mod utils;

use peripheral::{mailbox, uart};

extern crate alloc;
use alloc::vec::Vec;

const MAX_COMMAND_LEN: usize = 0x400;

#[no_mangle]
#[link_section = ".text.main"]
pub extern "C" fn main() {
    uart::init();
    stdio::puts(b"Hello, world!");
    let revision = mailbox::get_board_revision();
    stdio::write(b"Board revision: ");
    stdio::puts(utils::u32_to_hex(revision).as_ref());
    let (lb, ub) = mailbox::get_arm_memory();
    stdio::write(b"ARM memory: ");
    stdio::write(utils::u32_to_hex(ub).as_ref());
    stdio::write(b" ");
    stdio::puts(utils::u32_to_hex(lb).as_ref());

    stdio::puts(b"Reading initramfs...");
    let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
    loop {
        utils::memset(buf.as_mut_ptr(), 0, MAX_COMMAND_LEN);
        stdio::write(b"# ");
        stdio::gets(&mut buf);
        execute_command(&buf);
    }
    // let _a: Vec<u32> = Vec::new();
}

fn execute_command(command: &[u8]) {
    if command.starts_with(b"\x00") {
        return;
    } else if command.starts_with(b"hello") {
        stdio::puts(b"Hello, world!");
    } else if command.starts_with(b"help") {
        stdio::puts(b"hello\t: print this help menu");
        stdio::puts(b"help\t: print Hello World!");
        stdio::puts(b"reboot\t: reboot the Raspberry Pi");
    } else if command.starts_with(b"reboot") {
        peripheral::reboot::reboot();
    } else if command.starts_with(b"ls") {
        let rootfs = filesystem::cpio::CpioArchive::load(0x8000000 as *const u8);
        rootfs.print_file_list();
    } else if command.starts_with(b"cat") {
        let rootfs = filesystem::cpio::CpioArchive::load(0x8000000 as *const u8);
        let filename = &command[4..];
        if let Some(data) = rootfs.get_file(core::str::from_utf8(filename).unwrap()) {
            stdio::write(data);
        } else {
            stdio::write(b"File not found: ");
            stdio::puts(filename);
        }
    } else {
        stdio::write(b"Unknown command: ");
        stdio::puts(command);
    }
}
