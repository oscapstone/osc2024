#![no_std]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod mmio;
mod uart;
mod utils;
use mmio::mailbox::get_board_revision;

const MAX_COMMAND_LEN: usize = 0x400;

#[no_mangle]
pub extern "C" fn main() {
    unsafe {
        uart::uart_init();
        uart::uart_write(b"Hello, world!\n");
        get_board_revision();
        let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
        loop {
            utils::memset(buf.as_mut_ptr(), 0, MAX_COMMAND_LEN);
            uart::uart_write(b"# ");
            uart::uart_gets(&mut buf);
            execute_command(&buf);
        }
    }
}

fn execute_command(command: &[u8]) {
    unsafe {
        if utils::strncmp(command.as_ptr(), b"hello".as_ptr(), 5) == 0 {
            uart::uart_write(b"Hello, world!\n");
        } else if utils::strncmp(command.as_ptr(), b"help".as_ptr(), 4) == 0 {
            uart::uart_write(b"hello\t: print this help menu\n");
            uart::uart_write(b"help\t: print Hello World!\n");
            uart::uart_write(b"reboot\t: reboot the Raspberry Pi\n");
        } else if utils::strncmp(command.as_ptr(), b"reboot".as_ptr(), 6) == 0 {
            mmio::MMIO::reboot();
        } else {
            uart::uart_write(b"Unknown command\n");
        }
    }
}
