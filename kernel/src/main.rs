#![feature(asm_const)]
#![no_main]
#![no_std]
#![feature(default_alloc_error_handler)]

mod bsp;
mod cpu;
mod panic_wait;
mod allocator;

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
    uart::_print("Revision: ");
    // get board revision
    let revision = mailbox::get_board_revisioin();
    let h = alloc::string::String::from("Hello");


    // print hex of revision
    uart::print_hex(revision);
    uart::println("");

    // get ARM memory base address and size
    let (base, size) = mailbox::get_arm_memory();

    // print ARM memory base address and size
    uart::_print("ARM memory base address: ");
    uart::print_hex(base);
    uart::_print("\r\n");

    uart::_print("ARM memory size: ");
    uart::print_hex(size);
    uart::_print("\r\n");

    let mut out_buf: [u8; 128] = [0; 128];
    let mut in_buf: [u8; 128] = [0; 128];


    let mut handler: CpioHandler = CpioHandler::new(QEMU_INITRD_START as *mut u8);
    loop {
        uart::_print(">> ");
        let inp = uart::getline(&mut in_buf, true);
        if inp == "Hello" {
            uart::_print("Hello World!\r\n");
        } else if inp == "help" {
            uart::_print("help    : print this help menu\r\nhello   : print Hello World!\r\nreboot  : reboot the device\r\n");
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
                uart::_print(name);
                uart::_print("\r\n");
    
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
                    uart::_print("File: ");
                    uart::_print(name);
                    uart::_print("\r\n");
                    let contant =handler.read_current_file();
                    uart::_print(core::str::from_utf8(contant).unwrap());
                    uart::_print("\r\n");
                }
                handler.next_file();
            }
            handler.rewind();
        } 
        
        else {
            uart::_print("Command not found\r\n");
        }

        uart::uart_nops();
    }
    panic!()
}
