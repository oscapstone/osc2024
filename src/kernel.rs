mod mmio;
mod stdio;
mod uart;

#[no_mangle]
fn main() {
    uart::uart_init();
    stdio::puts(b"Hello, world!");
    loop {}
}
