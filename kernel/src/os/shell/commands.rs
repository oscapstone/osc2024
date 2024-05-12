use crate::os::{allocator, timer};
use crate::os::stdio::{get_line, print, println_now};
use crate::println;
use core::alloc::Layout;
use core::fmt::Display;
use core::arch::asm;
use alloc::boxed::Box;
use alloc::string::String;
use alloc::vec::Vec;


use super::INITRAMFS;

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
    println!("Timer expired");
    timer::add_timer_ms(2000, Box::new(|| set_next_timer()));
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
    let filename = args.get(1).unwrap_or(&String::new()).clone();
    match initramfs.load_file_to_memory(filename.as_str(), 0x2001_0000 as *mut u8) {
        true => println!("File loaded to memory"),
        false => {
            println!("File not found");
            return;
        }
    }

    timer::add_timer_ms(2000, Box::new(|| set_next_timer()));

    unsafe {
        asm!(
            "mov {tmp}, 0x200",
            "msr spsr_el1, {tmp}",
            "ldr {tmp}, =0x20010000", // Program counter
            "msr elr_el1, {tmp}",
            "ldr {tmp}, =0x2000F000", // Stack pointer
            "msr sp_el0, {tmp}",
            "eret",
            tmp = out(reg) _,
        );

        println!("Should not reach here");
    }
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
