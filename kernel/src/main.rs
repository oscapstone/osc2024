#![feature(asm_const)]
#![no_main]
#![no_std]
#![feature(default_alloc_error_handler)]
#![feature(format_args_nl)]
#![feature(int_log)]
#![feature(linked_list_cursors)]

mod bsp;
mod cpu;
mod fdt;
mod panic_wait;
mod print;
mod fs;
mod timer;
mod memory;

mod interrupt;

extern crate alloc;

use core::alloc::GlobalAlloc;
use core::panic;

use alloc::string::String;
use alloc::string::ToString;
use alloc::vec::Vec;
use alloc::collections::BinaryHeap;
use fs::cpio::CpioHandler;
use driver::mailbox;
use driver::uart;
use driver::addr_loader;
use memory::{dynmemalloc, simple_alloc::SimpleAllocator};
use core::alloc::Layout;

#[global_allocator]
static mut ALLOCATOR: SimpleAllocator = SimpleAllocator::new();

fn get_initrd_addr(name: &str, data: *const u8, len: usize) -> Option<u32> {
    if name == "linux,initrd-start" {
        unsafe { Some((*(data as *const u32)).swap_bytes()) }
    } else {
        None
    }
}

#[no_mangle]
#[inline(never)]
fn exec(addr: extern "C" fn() -> !) {
    // set spsr_el1 to 0x3c0 and elr_el1 to the program’s start address.
    // set the user pro gram’s stack pointer to a proper position by setting sp_el0.
    // issue eret to return to the user code.
    unsafe {
        core::arch::asm!("
        msr spsr_el1, {k}
        msr elr_el1, {a}
        msr sp_el0, {s}
        eret
        ", k = in(reg) 0x3c0 as u64, a = in(reg) addr as u64, s = in(reg) 0x60000 as u64) ;
    };
    println!("");
}

#[no_mangle]
#[inline(never)]
fn boink() {
    unsafe{core::arch::asm!("nop")};
}
        
fn timer_callback(message: String) {
    let current_time = timer::get_current_time() / timer::get_timer_freq();
    println!("You have a timer after boot {}s, message: {}", current_time, message);

}

#[no_mangle]
fn kernel_init() -> ! {
    let mut dtb_addr = addr_loader::load_dtb_addr();
    let dtb_parser = fdt::DtbParser::new(dtb_addr);
 
    // init uart
    uart::init_uart(true);
    uart::uart_write_str("Kernel started\r\n");
    
    // find initrd value by dtb traverse

    let mut initrd_start: *mut u8;
    if let Some(addr) = dtb_parser.traverse(get_initrd_addr) {
        initrd_start = addr as *mut u8;
        println!("Initrd start address: {:#x}", initrd_start as u64)
    } else {
        initrd_start = 0 as *mut u8;
    }
    let mut handler: CpioHandler = CpioHandler::new(initrd_start as *mut u8);
    let mut bh: BinaryHeap<u32> = BinaryHeap::new();
    
    // load symbol address usr_load_prog_base
    
    let mut in_buf: [u8; 128] = [0; 128];
    let mut alloc = dynmemalloc::DynMemAllocator::new();
 
    alloc.init(0x00000000, 0x3C000000 , 4096);

    alloc.reserve_addr(0x0, 0x1000);
    alloc.reserve_addr(0x60000, 0x80000); // stack
    alloc.reserve_addr(0x80000, 0x100000); // code
    alloc.reserve_addr(initrd_start as usize, initrd_start as usize + 4096); //initrd
    alloc.reserve_addr(dtb_addr as usize, ((dtb_addr as usize + dtb_parser.get_dtb_size() as usize) + (4096 - 1)) & !(4096 - 1)); // dtb

    loop {
        print!("meow>> ");
        let inp = alloc::string::String::from(uart::async_getline(&mut in_buf, true));
        let cmd = inp.trim().split(' ').collect::<Vec<&str>>();
        // split the input string
        match cmd[0] {
            "hello" => println!("Hello World!"),
            "help" => println!("help    : print this help menu\r\nhello   : print Hello World!\r\nreboot  : reboot the device"),
            "reboot" => {
                uart::reboot();
                break;
            }
            "ls" => {
                for file in handler.get_files() {
                    println!("{}, size: {}", file.get_name(), file.get_size());
                }
            }
            "cat" => {
                if cmd.len() < 2 {
                    println!("Usage: cat <file>");
                    continue;
                }
                let mut file = handler.get_files().find(|f| f.get_name() == cmd[1]);
                if let Some(mut f) = file {
                    println!("File size: {}", f.get_size());
                    loop {
                        let data = f.read(32);
                        if data.len() == 0 {
                            break;
                        }
                        for i in data {
                            unsafe {
                                uart::write_u8(*i);
                            }
                        }
                    }
                } else {
                    println!("File not found");
                }
            }
            "dtb" => {
                unsafe { println!("Start address: {:#x}", dtb_addr as u64) };
                println!("dtb load address: {:#x}", dtb_addr as u64);
                println!("dtb pos: {:#x}", unsafe {*(dtb_addr)});
                dtb_parser.parse_struct_block();
            }
            "mailbox" => {
                println!("Revision: {:#x}", mailbox::get_board_revisioin());
                let (base, size) = mailbox::get_arm_memory();
                println!("ARM memory base address: {:#x}", base);
                println!("ARM memory size: {:#x}", size);
            }
            "exec" => {
                let usr_load_prog_base = addr_loader::usr_load_prog_base();
                if cmd.len() < 2 {
                    println!("Usage: exec <file>");
                    continue;
                }
                let mut file = handler.get_files().find(|f| {
                    f.get_name() == cmd[1]}
                );
                if let Some(mut f) = file {
                    let file_size = f.get_size();
                    println!("File size: {}", file_size);
                    let mut data = f.read(file_size);
                    unsafe {
                        let mut addr = usr_load_prog_base as *mut u8;
                        for i in data {
                            *addr = *i;
                            addr = addr.add(1);
                        }
                        println!();
                        let func: extern "C" fn() -> ! = core::mem::transmute(usr_load_prog_base);
                        println!("Jump to user program at {:#x}", func as u64);
                        exec(func);
                    }
                } else {
                    println!("File not found");
                }
            }
            "setTimeout" => {
                if cmd.len() < 3 {
                    println!("Usage: setTimeout <time>(s) <message>");
                    continue;
                }
                let time: u64 = cmd[1].parse().unwrap();
                let mesg = cmd[2].to_string();
                println!("Set timer after {}s, message :{}", time, mesg);                
                timer::add_timer(timer_callback, time, mesg);
            }
            "dalloc" => {
                if cmd.len() < 3 {
                    println!("Usage: dalloc <size> <allign>");
                    continue;
                }
                let size = match cmd[1].parse() {
                    Ok(s) => s,
                    Err(_) => {
                        println!("Invalid size");
                        continue;
                    }
                };
                let allign = match cmd[2].parse() {
                    Ok(s) => s,
                    Err(_) => {
                        println!("Invalid allign");
                        continue;
                    }
                };

                let alloc_addr = unsafe {alloc.alloc(Layout::from_size_align(size, allign).unwrap())} as usize;
                println!("Allocated address: {:#x}", alloc_addr);
            }
            "dfree" => {
                if cmd.len() < 2 {
                    println!("Usage: dfree <addr>");
                    continue;
                }
                let size: usize = cmd[1].parse().unwrap();
                unsafe {alloc.dealloc(size as *mut u8, Layout::from_size_align(1, 1).unwrap())};
            }
            "palloc" => {
                let size: usize = cmd[1].parse().unwrap();
                let pn = unsafe {alloc.palloc(size)} as usize;
                println!("Allocated page: {}", pn);
            }
            "pfree" => {
                let addr: usize = cmd[1].parse().unwrap();
                unsafe {alloc.pfree(addr)};
            }
            "show" => {
                if cmd.len() < 2 {
                    println!("Usage: show <d/p>");
                    continue;
                }   
                match cmd[1] {
                    "d" => {
                        alloc.dshow();
                    }
                    "p" => {
                        alloc.pshow();
                    }
                    _ => {
                        println!("Usage: show <d/p>");
                    }
                }
            }
            "rsv" => {
                if cmd.len() < 3 {
                    println!("Usage: rsv <start> <end>");
                    continue;
                }
                let start: usize = cmd[1].parse().unwrap();
                let end: usize = cmd[2].parse().unwrap();
                unsafe {alloc.reserve_addr(start, end)};
            }
            "" => {}
            _ => {
                println!("Shell: Command not found");
            }
        }
    }


    panic!()
}
