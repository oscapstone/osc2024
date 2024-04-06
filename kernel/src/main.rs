#![feature(asm_const)]
#![no_main]
#![no_std]
#![feature(default_alloc_error_handler)]
#![feature(format_args_nl)]

mod allocator;
mod bsp;
mod cpu;
mod fdt;
mod panic_wait;
mod print;

extern crate alloc;

use alloc::vec::Vec;
use allocator::MyAllocator;
use driver::cpio::CpioHandler;
use driver::mailbox;
use driver::uart;
use driver::addr_loader;

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
    let mut dtb_addr = addr_loader::load_dtb_addr();
    // init uart
    uart::init_uart();

    // println!("DTB address: {:#x}", dtb_pos as u64);
    let dtb_parser = fdt::DtbParser::new(dtb_addr);

    let mut in_buf: [u8; 128] = [0; 128];
    // find initrd value by dtb traverse

    let mut initrd_start: *mut u8;
    if let Some(addr) = dtb_parser.traverse(get_initrd_addr) {
        initrd_start = addr as *mut u8;
        println!("Initrd start address: {:#x}", initrd_start as u64)
    } else {
        initrd_start = 0 as *mut u8;
    }

    let mut handler: CpioHandler = CpioHandler::new(initrd_start as *mut u8);
    loop {
        print!("# ");
        let inp = alloc::string::String::from(uart::getline(&mut in_buf, true));
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
                for name in handler.list_all_files() {
                    println!("{}", name);
                }
            }
            "cat" => {
                if cmd.len() < 2 {
                    println!("Usage: cat <filename>");
                    continue;
                }
                let target = cmd[1];
                loop {
                    if let Some(name) = handler.get_current_file_name() {
                        if name == target {
                            println!("File: {}", name);
                            let content = handler.read_current_file();
                            println!("{}", core::str::from_utf8(content).unwrap());
                            break;
                        }
                        handler.next_file();
                    }
                    else {
                        println!("File not found");
                        break;
                    }
                }
                handler.rewind();
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
            "" => {
                println!("");
            }
            _ => {
                println!("Command not found\r\n");
            }
        }
    }
    panic!()
}
