#![no_std]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod mmio;
mod uart;
mod utils;

const MAX_COMMAND_LEN: usize = 1024;

#[no_mangle]
pub extern "C" fn main() {
    unsafe {
        uart::uart_init();
        uart::uart_write(b"Hello, world!\n");
        let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
        let mut len = 0usize;
        loop {
            unsafe {
                utils::memset(buf.as_mut_ptr(), 0, MAX_COMMAND_LEN);
                uart::uart_write(b"> ");
            }
            loop {
                let input = uart::uart_recv();
                uart::uart_send(input);
                if input == b'\n' {
                    if len > 0 {
                        uart::uart_puts(&buf);
                        len = 0;
                    }
                    break;
                } else if len < MAX_COMMAND_LEN {
                    buf[len] = input;
                    len += 1;
                }
            }
        }
    }
}
