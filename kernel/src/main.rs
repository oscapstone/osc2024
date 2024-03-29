#![feature(asm_const)]
#![no_main]
#![no_std]
#![feature(default_alloc_error_handler)]
#![feature(format_args_nl)]

mod bsp;
mod cpu;
mod panic_wait;
mod allocator;
mod print;

extern crate alloc; 

use core::ptr::write_volatile;
use driver::cpio::CpioHandler;
use driver::mailbox;
use driver::uart;
use allocator::MyAllocator;

const QEMU_INITRD_START: u64 = 0x8000000;

#[global_allocator]
static mut ALLOCATOR: MyAllocator = MyAllocator::new();

#[no_mangle]
unsafe fn kernel_init() -> ! {
    
    uart::init_uart();
    println!("Revision: {:#x}", mailbox::get_board_revisioin());
    // get board revision
    let revision = mailbox::get_board_revisioin();

    // print hex of revision

    // get ARM memory base address and size
    let (base, size) = mailbox::get_arm_memory();

    // print ARM memory base address and size
    println!("ARM memory base address: {:#x}", base);

    println!("ARM memory size: {:#x}", size);

    let mut out_buf: [u8; 128] = [0; 128];
    let mut in_buf: [u8; 128] = [0; 128];


    let mut handler: CpioHandler = CpioHandler::new(QEMU_INITRD_START as *mut u8);
    loop {
        uart::uart_write_str(">> ");
        let inp = uart::getline(&mut in_buf, true);
        if inp == "Hello" {
            uart::uart_write_str("Hello World!\r\n");
        } else if inp == "help" {
            uart::uart_write_str("help    : print this help menu\r\nhello   : print Hello World!\r\nreboot  : reboot the device\r\n");
        } else if inp == "reboot" {
            const PM_PASSWORD: u32 = 0x5a000000;
            const PM_RSTC: u32 = 0x3F10001c;
            const PM_WDOG: u32 = 0x3F100024;
            const PM_RSTC_WRCFG_FULL_RESET: u32 = 0x00000020;
            write_volatile(PM_WDOG as *mut u32, PM_PASSWORD | 100);
            write_volatile(PM_RSTC as *mut u32, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
            break;
        } else if inp == "ls" {
            loop {
                let name = handler.get_current_file_name();
                if name == "TRAILER!!!\0" {
                    break;
                }
                uart::uart_write_str(name);
                uart::uart_write_str("\r\n");
    
                handler.next_file();
            }
            handler.rewind();
        } else if uart::strncmp(inp, "cat", 3){
            let target = &inp[4..];
            loop {
                let name = handler.get_current_file_name();
                if name == "TRAILER!!!\0" {
                    break;
                }
                let l = name.len();
                // if last char is \0, remove it
                let name = if name.as_bytes()[l-1] == 0 {
                    &name[..l-1]
                } else {
                    name
                };
                if name == target {
                    uart::uart_write_str("File: ");
                    uart::uart_write_str(name);
                    uart::uart_write_str("\r\n");
                    let contant =handler.read_current_file();
                    uart::uart_write_str(core::str::from_utf8(contant).unwrap());
                    uart::uart_write_str("\r\n");
                }
                handler.next_file();
            }
            handler.rewind();
        } 
        
        else {
            uart::uart_write_str("Command not found\r\n");
        }

        uart::uart_nops();
    }
    panic!()
}
