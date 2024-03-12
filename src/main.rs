#![no_std]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod mmio;
mod uart;
mod utils;
use mmio::mailbox::get_arm_memory;
use mmio::mailbox::get_board_revision;

const MAX_COMMAND_LEN: usize = 0x400;

#[no_mangle]
pub fn main() {
    uart::uart_init();
    uart::uart_puts(b"Hello, world!");
    get_board_revision();
    get_arm_memory();
    let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
    loop {
        utils::memset(buf.as_mut_ptr(), 0, MAX_COMMAND_LEN);
        uart::uart_write(b"# ");
        uart::uart_gets(&mut buf);
        execute_command(&buf);
    }
}

fn execute_command(command: &[u8]) {
    if command.starts_with(b"\x00") {
        return;
    } else if command.starts_with(b"hello") {
        uart::uart_puts(b"Hello, world!");
    } else if command.starts_with(b"help") {
        uart::uart_puts(b"hello\t: print this help menu");
        uart::uart_puts(b"help\t: print Hello World!");
        uart::uart_puts(b"reboot\t: reboot the Raspberry Pi");
    } else if command.starts_with(b"reboot") {
        mmio::MMIO::reboot();
    } else {
        uart::uart_write(b"Unknown command: ");
        uart::uart_puts(command);
    }
}
