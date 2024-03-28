#![no_std]
#![no_builtins]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod allocator;
mod dtb;
mod filesystem;
mod mmio;
mod peripheral;
mod stdio;
mod utils;

extern crate alloc;

const MAX_COMMAND_LEN: usize = 0x400;

static mut INITRAMFS_ADDR: u32 = 0;

#[no_mangle]
#[link_section = ".text.main"]
pub extern "C" fn main() {
    peripheral::uart::init();
    stdio::puts(b"Hello, world!");
    let revision = peripheral::mailbox::get_board_revision();
    stdio::write(b"Board revision: ");
    stdio::puts(utils::u32_to_hex(revision).as_ref());
    let (lb, ub) = peripheral::mailbox::get_arm_memory();
    stdio::write(b"ARM memory: ");
    stdio::write(utils::u32_to_hex(ub).as_ref());
    stdio::write(b" ");
    stdio::puts(utils::u32_to_hex(lb).as_ref());

    unsafe {
        INITRAMFS_ADDR = dtb::get_initrd_start().unwrap();
    }

    stdio::println("Dealing with dtb...");

    let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
    loop {
        utils::memset(buf.as_mut_ptr(), 0, MAX_COMMAND_LEN);
        stdio::write(b"# ");
        stdio::gets(&mut buf);
        execute_command(&buf);
    }
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
        peripheral::watchdog::reset(100);
    } else if command.starts_with(b"ls") {
        let rootfs = filesystem::cpio::CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
        rootfs.print_file_list();
    } else if command.starts_with(b"cat") {
        let rootfs = filesystem::cpio::CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
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
