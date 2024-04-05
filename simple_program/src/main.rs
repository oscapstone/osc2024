#![no_main]
#![no_std]

mod cpu;
mod os;
extern crate alloc;

#[no_mangle]
fn main() {
    println!("Hello, world!");
    loop {}
}
