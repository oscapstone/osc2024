#![feature(asm_const)]
#![no_main]
#![no_std]

mod bsp;
mod cpu;
mod panic_wait;


use core::{
    ptr::{read_volatile, write_volatile},
    usize,
};

use driver::uart;
use driver::mailbox;

const KERNEL_LOAD_ADDR: u64 = 0x80000;


// load the kernel from uart
#[no_mangle]
unsafe fn bootloader() -> ! {
    // initialize uart
    uart::init_uart();
    uart::_print("Bootloader started\r\n");

    // read kernel size
    let mut kernel_size: u32 = 0;
    
    // read kernel size (4 bytes) from uart
    uart::read(&mut kernel_size as *mut u32 as *mut u8 /*(uint8_t)(&kernel_size) */, 4);
    uart::_print("Kernel size: ");
    uart::print_hex(kernel_size);
    uart::_print("\r\n");

    // start to read kernel
    let mut kernel_addr: u64 = KERNEL_LOAD_ADDR;
    let mut read_size: u32 = 0;
    while read_size < kernel_size {
        let mut read_buf: [u8; 128] = [0; 128];
        let mut read_len: usize = 128;
        if kernel_size - read_size < 128 {
            read_len = (kernel_size - read_size) as usize;
        }
        uart::read(read_buf.as_mut_ptr(), read_len);
        for i in 0..read_len {
            write_volatile(kernel_addr as *mut u8, read_buf[i]);
            kernel_addr += 1;
        }
        read_size += read_len as u32;
    }

    // jump to kernel
    uart::_print("Jump to kernel\r\n");
}
