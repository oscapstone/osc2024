#![no_main]
#![no_std]

mod os;
use core::arch::{asm, global_asm};
use crate::os::system_call;

// extern crate alloc;

global_asm!(include_str!("boot.s"));

#[no_mangle]
fn main() {
    let mut buf = [0u8; 256];
    loop {
        let size = system_call::uart_read(buf.as_mut_ptr(), buf.len());
        system_call::uart_write(buf.as_ptr(), size);
        // println!("Read {} bytes", size);
        // println!("Data: {:?}", core::str::from_utf8(&buf[..size]));
    }
    loop {}
}