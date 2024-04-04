#![feature(asm_const)]
#![no_main]
#![no_std]
#![feature(default_alloc_error_handler)]
#![feature(format_args_nl)]

mod allocator;
mod bsp;
mod cpu;
mod dtb;
mod panic_wait;
mod print;

extern crate alloc;

use fdt;

use allocator::MyAllocator;
use core::ptr::addr_of;
use core::ptr::write_volatile;
use driver::cpio::CpioHandler;
use driver::mailbox;
use driver::uart;

const QEMU_INITRD_START: u64 = 0x8000000;

#[global_allocator]
static mut ALLOCATOR: MyAllocator = MyAllocator::new();

fn get_initrd_addr(name: &str, data: *const u8, len: usize) -> Option<u32> {
    if name == "linux,initrd-start" {
        unsafe { Some((*(data as *const u32)).swap_bytes()) }
    } else {
        None
    }
}

#[no_mangle]
fn kernel_init() -> ! {
    uart::init_uart();
    // load device tree at symbol __dtb_addr
    let mut dtb_addr: *mut u64;
    let mut dtb_pos: *mut u8;
    unsafe {
        core::arch::asm!("ldr {}, =__dtb_addr", out(reg) dtb_addr);
        dtb_pos = (*dtb_addr) as *mut u8;
    }
    let dtb_parser = dtb::DtbParser::new(dtb_pos);

    let f = unsafe { fdt::Fdt::from_ptr(dtb_pos as *const u8).unwrap() };

    let s = unsafe {
        core::slice::from_raw_parts(dtb_pos.add(dtb_parser.off_dt_strings as usize), 100)
    };

    let mut in_buf: [u8; 128] = [0; 128];
    // find initrd value by dtb traverse

    let mut initrd_start: *mut u8;
    if let Some(addr) = dtb_parser.traverse(get_initrd_addr) {
        initrd_start = addr as *mut u8;
    } else {
        initrd_start = 0 as *mut u8;
    }

    
    loop {
        
        println!("Initrd start: {:#x}", initrd_start as u64);
        print!(">> ");
        let inp = uart::getline(&mut in_buf, true);
        if inp == "hello" {
            println!("Hello World!");
        } else if inp == "help" {
            println!("help    : print this help menu\r\nhello   : print Hello World!\r\nreboot  : reboot the device");
        } else if inp == "reboot" {
            const PM_PASSWORD: u32 = 0x5a000000;
            const PM_RSTC: u32 = 0x3F10001c;
            const PM_WDOG: u32 = 0x3F100024;
            const PM_RSTC_WRCFG_FULL_RESET: u32 = 0x00000020;
            unsafe {
                write_volatile(PM_WDOG as *mut u32, PM_PASSWORD | 100);
                write_volatile(PM_RSTC as *mut u32, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
            }
            break;
        } else if inp == "ls" {
            let mut handler: CpioHandler = CpioHandler::new(initrd_start as *mut u8);

            loop {
                let name = handler.get_current_file_name();
                if name == "TRAILER!!!\0" {
                    break;
                }
                println!("{}", name);

                handler.next_file();
            }
            handler.rewind();
        } else if uart::strncmp(inp, "cat", 3) {
            let mut handler: CpioHandler = CpioHandler::new(initrd_start as *mut u8);

            let target = &inp[4..];
            loop {
                let name = handler.get_current_file_name();
                if name == "TRAILER!!!\0" {
                    break;
                }
                let l = name.len();
                // if last char is \0, remove it
                let name = if name.as_bytes()[l - 1] == 0 {
                    &name[..l - 1]
                } else {
                    name
                };
                if name == target {
                    println!("File: {}", name);
                    let content = handler.read_current_file();
                    println!("{}", core::str::from_utf8(content).unwrap());
                }
                handler.next_file();
            }
            handler.rewind();
        } else if inp == "dtb" {
            dtb_parser.parse_struct_block();
        } else if inp == "mailbox" {
            println!("Revision: {:#x}", mailbox::get_board_revisioin());
            let (base, size) = mailbox::get_arm_memory();
            println!("ARM memory base address: {:#x}", base);
            println!("ARM memory size: {:#x}", size);
        } else {
            println!("Command not found\r\n");
        }
        unsafe {
            uart::uart_nops();
        }
    }
    panic!()
}
