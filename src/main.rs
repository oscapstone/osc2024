#![feature(asm_const)]
#![no_main]
#![no_std]

use crate::uart::strcmp;
use core::{
    ptr::{read_volatile, write_volatile},
    usize,
};

mod bsp;
mod cpu;
mod panic_wait;
mod uart;
mod mailbox;

#[no_mangle]
unsafe fn kernel_init() -> ! {
    uart::init_uart();

    for i in b"Revision: " {
        uart::write_char(*i);
    }

    // get board revision
    let revision = mailbox::get_board_revisioin();

    // convert revision to string
    let mut rev_str: [u8; 128] = [0; 128];
    let mut rev_str_len = 0;

    // print hex of revision
    uart::print_hex(revision);

    uart::write_char(b'\n');
    uart::write_char(b'\r');

    // get ARM memory base address and size
    let (base, size) = mailbox::get_arm_memory();
    
    // print ARM memory base address and size
    for i in b"ARM memory base address: " {
        uart::write_char(*i);
    }
    uart::print_hex(base);
    uart::write_char(b'\n');
    uart::write_char(b'\r');
    
    for i in b"ARM memory size: " {
        uart::write_char(*i);
    }
    uart::print_hex(size);
    uart::write_char(b'\n');
    uart::write_char(b'\r');



    let mut out_buf: [u8; 128] = [0; 128];
    let mut in_buf: [u8; 128] = [0; 128];
    let out_buf_len: usize = 2;
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

    let mut hello_world_str: [u8; 128] = [0; 128];

    let hello_world_len = 1010;

    loop {
        uart::print_str(&out_buf, out_buf_len);
        let in_buf_len = uart::get_line(&mut in_buf, true);
        if strcmp(&in_buf, in_buf_len, &hello_str, hello_len) {
            for i in b"Hello World!\r\n" {
                uart::write_char(*i);
            }
            uart::write_char(10);
        } else if strcmp(&in_buf, in_buf_len, &help_str, help_len) {
            
            for i in b"help    : print this help menu\r\nhello   : print Hello World!\r\nreboot  : reboot the device\r\n" {
                uart::write_char(*i);
            }
            uart::write_char(10);
        } else if strcmp(&in_buf, in_buf_len, &reboot_str, 6) {

            const PM_PASSWORD: u32 = 0x5a000000;
            const PM_RSTC: u32 = 0x3F10001c;
            const PM_WDOG: u32 = 0x3F100024;
            const PM_RSTC_WRCFG_FULL_RESET: u32 = 0x00000020;
            write_volatile(PM_WDOG as *mut u32, PM_PASSWORD | 100);
            write_volatile(PM_RSTC as *mut u32, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
            break;
        }
        uart::nops();
    }
    panic!()
}
