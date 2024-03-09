#![no_std]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod uart;

#[no_mangle]
pub extern "C" fn main() {
    unsafe {
        uart::uart_init();
        uart::uart_write(b"Hello, world!\n");
        loop {
            let mut input: [u8; 1] = [0; 1];
            uart::uart_read(&mut input);
            uart::uart_write(&input);
        }
    }
}
