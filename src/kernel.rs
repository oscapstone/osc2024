#![no_std]

#[panic_handler]
pub extern "Rust" fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

mod mailbox;
mod mmio;
mod stdio;
mod uart;
mod utils;

// use mailbox;

const MAX_COMMAND_LEN: usize = 0x400;

#[no_mangle]
#[link_section = ".text.main"]
pub extern "C" fn main() {
    uart::uart_init();
    stdio::puts(b"Hello, world!");
    let revision = mailbox::get_board_revision();
    stdio::write(b"Board revision: ");
    stdio::puts(utils::to_hex(revision).as_ref());
    let (lb, ub) = mailbox::get_arm_memory();
    stdio::write(b"ARM memory: ");
    stdio::write(utils::to_hex(ub).as_ref());
    stdio::write(b" ");
    stdio::puts(utils::to_hex(lb).as_ref());

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
        mmio::MMIO::reboot();
    } else if command.starts_with(b"load") {
        stdio::write(b"Kernel image size: ");
        let buf = &mut [0u8; 16];
        stdio::gets(buf);
        let size = utils::atoi(buf);
        let kernel = 0x80000 as *const ();
        for i in 0..size {
            let c = uart::recv();
            unsafe {
                core::ptr::write_volatile(kernel.byte_offset(i as isize) as *mut u8, c);
            }
        }
        // jump to kernel
        let kernel: fn() = unsafe { core::mem::transmute(kernel) };
        kernel();
    } else {
        stdio::write(b"Unknown command: ");
        stdio::puts(command);
    }
}
