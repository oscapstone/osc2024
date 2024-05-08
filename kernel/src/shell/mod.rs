
use alloc::string::String;
use alloc::vec::Vec;
use driver::{mailbox, addr_loader, uart};
use crate::{println, print};
use crate::fs::cpio::CpioHandler;
use crate::alloc::string::ToString;
use crate::timer;

fn exec(addr: extern "C" fn() -> !) {
    // set spsr_el1 to 0x3c0 and elr_el1 to the program’s start address.
    // set the user program’s stack pointer to a proper position by setting sp_el0.
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

fn timer_callback(message: String) {
    let current_time = timer::get_current_time() / timer::get_timer_freq();
    println!("You have a timer after boot {}s, message: {}", current_time, message);
}

pub fn shell(handler: CpioHandler) {
    let mut in_buf: [u8; 128] = [0; 128];
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
                let file = handler.get_files().find(|f| f.get_name() == cmd[1]);
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
            // "dtb" => {
            //     unsafe { println!("Start address: {:#x}", dtb_addr as u64) };
            //     println!("dtb load address: {:#x}", dtb_addr as u64);
            //     println!("dtb pos: {:#x}", unsafe {*(dtb_addr)});
            //     dtb_parser.parse_struct_block();
            // }
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
                let file = handler.get_files().find(|f| {
                    f.get_name() == cmd[1]}
                );
                if let Some(mut f) = file {
                    let file_size = f.get_size();
                    println!("File size: {}", file_size);
                    let data = f.read(file_size);
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
            "" => {}
            _ => {
                println!("Shell: Command not found");
            }
        }
    }
}