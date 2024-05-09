use crate::os::allocator;
use crate::os::stdio::{get_line, print, println_now};
use crate::println;
use core::alloc::Layout;

pub fn help() {
    println!("help\t:print this help menu");
    println!("hello\t:print Hello, World!");
    println!("reboot\t:reboot the device");
    println!("ls\t:list files in the initramfs");
    println!("cat\t:print the content of a file in the initramfs");
    println!("exec\t:load a file to memory and execute it");
    println!("test_memory\t:test buddy memory allocation");
}

pub fn test_memory() {
    let mut inp_buf = [0u8; 256];
    println!("Usage:");
    println!("malloc <size> -> <address> - Allocate memory");
    println!("free <address> <size> - Free memory");
    println!("");
    loop {
        print!("test_mem > ");
        let len = get_line(&mut inp_buf, 256);
        let inp_buf = &inp_buf[..len - 1];
        if inp_buf.starts_with(b"malloc") {
            let mut iter = inp_buf.split(|c| c.clone() == b' ');
            assert_eq!(iter.next().unwrap(), b"malloc");
            let size = core::str::from_utf8(iter.next().unwrap()).unwrap();
            match size.parse::<usize>() {
                Ok(size) => {
                    let layout = Layout::from_size_align(size, 4).unwrap();
                    println_now("");
                    allocator::set_print_debug(true);
                    let ptr = unsafe { alloc::alloc::alloc(layout) };
                    allocator::set_print_debug(false);
                    println!("Pointer address: {:X?}", ptr);
                }
                Err(_) => {
                    println!("Invalid size: {}", size);
                }
            }
        } else if inp_buf.starts_with(b"free") {
            let mut iter = inp_buf.split(|c| c.clone() == b' ');
            assert_eq!(iter.next().unwrap(), b"free");
            let address = core::str::from_utf8(iter.next().unwrap()).unwrap();
            let size = core::str::from_utf8(iter.next().unwrap()).unwrap();

            let address = match usize::from_str_radix(address, 16) {
                Ok(address) => address,
                Err(_) => {
                    println!("Invalid address: {}", address);
                    continue;
                }
            } as *mut u8;

            let size = match size.parse::<usize>() {
                Ok(size) => size,
                Err(_) => {
                    println!("Invalid size: {}", size);
                    continue;
                }
            };

            let layout = Layout::from_size_align(size, 4).unwrap();

            println_now("");
            allocator::set_print_debug(true);
            let result = unsafe { alloc::alloc::dealloc(address, layout) };
            allocator::set_print_debug(false);

            println!("Freed");
        } else if inp_buf.starts_with(b"exit") {
            break;
        } else {
            println!("Unknown command");
        }
    }
}
