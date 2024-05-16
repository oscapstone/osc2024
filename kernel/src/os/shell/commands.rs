use crate::os::stdio::{get_line, print, println_now};
use crate::os::{allocator, thread, timer};
use crate::println;
use alloc::boxed::Box;
use alloc::string::String;
use alloc::vec::Vec;
use core::alloc::Layout;
use core::arch::asm;
use core::fmt::Display;

use super::INITRAMFS;

const CONTEXT_SWITCHING_DELAY: u64 = 1000/32;

pub struct command {
    name: &'static str,
    description: &'static str,
    function: fn(Vec<String>),
}

impl Display for command {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", self.name)
    }
}

fn set_next_timer() {
    // println!("Timer context switching");
    thread::context_switching();
    timer::add_timer_ms(CONTEXT_SWITCHING_DELAY, Box::new(|| set_next_timer()));
}

impl command {
    pub fn new(name: &'static str, description: &'static str, function: fn(Vec<String>)) -> Self {
        command {
            name,
            description,
            function,
        }
    }

    pub fn get_name(&self) -> &'static str {
        self.name
    }

    pub fn get_description(&self) -> &'static str {
        self.description
    }

    pub fn execute(&self, args: Vec<String>) {
        (self.function)(args);
    }
}

pub fn hello(args: Vec<String>) {
    println!("Hello, World!");
}

pub fn reboot(args: Vec<String>) {
    println!("Rebooting...");
    crate::cpu::reboot::reset(100);
    loop {}
}

// TODO - Implement the help function
pub fn help(args: Vec<String>) {
    todo!("Implement the help function");
}

pub fn ls(args: Vec<String>) {
    let initramfs = unsafe { INITRAMFS.as_ref().unwrap() };
    for file in initramfs.get_file_list() {
        println!(file);
    }
}

pub fn cat(args: Vec<String>) {
    let args = args.iter().skip(1);
    let initramfs = unsafe { INITRAMFS.as_ref().unwrap() };
    for arg in args {
        initramfs.print_file_content(arg);
    }
}

pub fn exec(args: Vec<String>) {
    let initramfs = unsafe { INITRAMFS.as_ref().unwrap() };

    for filename in args.iter().skip(1) {
        let filesize = match initramfs.get_filesize_by_name(filename.as_str()) {
            Some(size) => size as usize,
            None => {
                println!("File not found");
                return;
            }
        };
        let stack_size = 4096;

        let program_ptr =
            unsafe { alloc::alloc::alloc(Layout::from_size_align(filesize, 32).unwrap()) };
        let program_stack_ptr =
            unsafe { alloc::alloc::alloc(Layout::from_size_align(stack_size, 32).unwrap()) };

        for i in 0..filesize {
            unsafe {
                core::ptr::write(program_ptr.add(i), 0);
            }
        }

        match initramfs.load_file_to_memory(filename.as_str(), program_ptr) {
            true => println!("File loaded to memory"),
            false => {
                println!("File not found");
                return;
            }
        }
        
        let pid = thread::create_thread(program_ptr, filesize, program_stack_ptr, stack_size);
        println!("PID: {}", pid);
        println!("PC: {:X?}", program_ptr);
    }
    timer::add_timer_ms(CONTEXT_SWITCHING_DELAY, Box::new(|| set_next_timer()));
    thread::run_thread(None);

    panic!("Should not reach here");
}

pub fn set_timeout(args: Vec<String>) {
    let message = args.get(1).unwrap().clone();
    let time = args.get(2).unwrap();
    match time.parse::<u64>() {
        Ok(time) => {
            timer::add_timer_ms(time, move || println!("{}", message));
        }
        Err(_) => {
            println!("Invalid time: {}", time.len());
        }
    }
}

pub fn test_memory(args: Vec<String>) {
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

pub fn current_el(args: Vec<String>) {
    let current_el: u64;
    unsafe {
        asm!("mrs {0}, CurrentEL", out(reg) current_el);
    }
    println!("CurrentEL: {}", (current_el >> 2) & 0b11);
}