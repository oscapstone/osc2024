extern crate alloc;

use alloc::boxed::Box;
use core::ptr;

use crate::{arrsting::ArrString, console::console, mbox, power, print, println};


pub fn start_shell() -> !{
    let msg_buf_exceed = "[system] Buf size limit exceed, reset buf";

    let msg_help = "help\t: print this help menu\r\nhello\t: print Hello World!\r\nreboot\t: reboot the device\r\ninfo\t: show the device info";
    let msg_hello_world = "HelloWorld!";
    let msg_not_found = "Command not found";
    let msg_reboot = "Rebooting...";

    let arr_help = ArrString::new("help");
    let arr_hello = ArrString::new("hello");
    let arr_reboot = ArrString::new("reboot");
    let arr_info = ArrString::new("info");
    let msg_alloc_test=ArrString::new("alloc");

    let mut buf = ArrString::new("");

    println!("TEST VER 0.0.2\r\n");
    print!("{}\r\n#", msg_help);

    loop {
        let c = console().read_char();
        console().write_char(c);

        if c == '\n' {
            print!("\r");
            if buf == arr_help {
                println!("{}", msg_help);
            } else if buf == arr_hello {
                println!("{}", msg_hello_world);
            } else if buf == arr_reboot {
                println!("{}", msg_reboot);
                power::reboot();
            } else if buf == arr_info {
                println!("BoardVersion: {:x}", mbox::mbox().get_board_revision());
                // println!("BoardVersion: {:x}", bsp::driver::MBOX.get_board_revision());
                println!(
                    "RAM: {} {}",
                    mbox::mbox().get_arm_memory().0,
                    mbox::mbox().get_arm_memory().1
                );
            } else if buf == msg_alloc_test {
                println!("Allocate test start!");
                check_alloc();
                println!("Allocate test End!");
            } else {
                println!("{}", msg_not_found);
            }

            buf.clean_buf();
            print!("#");
            continue;

            // arrsting::arrstrcmp(buf, help);
        } else if buf.get_len() == 1024 {
            buf.clean_buf();
            println!("{}", msg_buf_exceed);
            print!("#");
            continue;
        } else {
            buf.push_char(c);
        }
    }
}

// This is a test for checking memory allocation
fn check_alloc() {
    let long_lived = Box::new(1); // new
    for i in 0..32 {
        let x = Box::new(i);
        println!("{}", i);
        println!{"{:p}" ,ptr::addr_of!(*x)};
        assert_eq!(*x, i);
    }
    assert_eq!(*long_lived, 1); // new
}