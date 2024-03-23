#![feature(asm_const)]
#![no_main]
#![no_std]

mod bsp;
mod cpu;
mod panic_wait;

use core::ptr::write_volatile;

use driver::uart;
use driver::mailbox;

#[no_mangle]
unsafe fn kernel_init() -> ! {
    uart::init_uart();
    uart::_print("Revision: ");
    // get board revision
    let revision = mailbox::get_board_revisioin();

    // print hex of revision
    uart::print_hex(revision);
    uart::println("");

    // get ARM memory base address and size
    let (base, size) = mailbox::get_arm_memory();
    
    // print ARM memory base address and size
    uart::_print("ARM memory base address: ");
    uart::print_hex(base);
    uart::_print("\r\n");
    

    uart::_print("ARM memory size: ");
    uart::print_hex(size);
    uart::_print("\r\n");

    let mut out_buf: [u8; 128] = [0; 128];
    let mut in_buf: [u8; 128] = [0; 128];
    out_buf[0] = 62;
    out_buf[1] = 62;

    let mut hello_str: [u8; 128] = [0; 128];
    hello_str[..5].copy_from_slice(b"hello");
    let hello_len = 5;

    let mut reboot_str: [u8; 128] = [0; 128];
    reboot_str[..6].copy_from_slice(b"reboot");

    let mut help_str: [u8; 128] = [0; 128];
    help_str[..4].copy_from_slice(b"help");
    let help_len = 4;

    loop {
        uart::_print(">> ");
        let in_buf_len = uart::get_line(&mut in_buf, true);
        if uart::strcmp(&in_buf, in_buf_len, &hello_str, hello_len) {
            uart::_print("Hello World!\r\n");
        } else if uart::strcmp(&in_buf, in_buf_len, &help_str, help_len) {
            uart::_print("help    : print this help menu\r\nhello   : print Hello World!\r\nreboot  : reboot the device\r\n");
        } else if uart::strcmp(&in_buf, in_buf_len, &reboot_str, 6) {

            const PM_PASSWORD: u32 = 0x5a000000;
            const PM_RSTC: u32 = 0x3F10001c;
            const PM_WDOG: u32 = 0x3F100024;
            const PM_RSTC_WRCFG_FULL_RESET: u32 = 0x00000020;
            write_volatile(PM_WDOG as *mut u32, PM_PASSWORD | 100);
            write_volatile(PM_RSTC as *mut u32, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
            break;
        }
        uart::uart_nops();
    }
    panic!()
}
