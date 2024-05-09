extern crate alloc;

use alloc::{boxed::Box, string::String};
use core::ptr;

use crate::{console::console, device_tree, fs, mbox, power, print, println};

pub fn start_shell() -> ! {
    println!("TEST VER 0.0.3\r\n");
    print_help();

    let mut input_string = String::from("");

    loop {
        let c = console().read_char();
        console().write_char(c);
        input_string.push(c);
        // println!("[shell] input_string: {}", input_string);

        if c == '\r' || c == '\n' {
            let (cmd, arg1) = input_string.split_once(char::is_whitespace).unwrap();
            let arg1 = arg1.trim_start();
            // println!("[shell] cmd: {}, arg1: {}", cmd, arg1);
            print!("\r");

            match cmd {
                "help" => {
                    print_help();
                }
                "hello" => {
                    println!("Hello World!");
                }
                "reboot" => {
                    println!("Rebooting...");
                    power::reboot();
                }
                "info" => {
                    println!("BoardVersion: {:x}", mbox::mbox().get_board_revision());
                    // println!("BoardVersion: {:x}", bsp::driver::MBOX.get_board_revision());
                    println!(
                        "RAM: {} {}",
                        mbox::mbox().get_arm_memory().0,
                        mbox::mbox().get_arm_memory().1
                    )
                }
                "alloc" => {
                    println!("Allocate test start!");
                    check_alloc();
                    println!("Allocate test End!");
                }
                "ls" => {
                    let fs: fs::init_ram_fs::Cpio = fs::init_ram_fs::Cpio::load(
                        device_tree::get_initrd_start().unwrap() as *const u8,
                    );
                    fs.print_file_list();
                }
                "cat" => {
                    // memory::get_initramfs_files(arg_1);
                    let fs: fs::init_ram_fs::Cpio = fs::init_ram_fs::Cpio::load(
                        device_tree::get_initrd_start().unwrap() as *const u8,
                    );
                    if let Some(data) = fs.get_file(arg1) {
                        print!("{}", core::str::from_utf8(data).unwrap());
                    } else {
                        println!("File not found: {}", arg1);
                    }
                }
                _ => {
                    println!("Unknown command: {:?}", cmd);
                }
            }
            input_string.clear();
            print!("#");
        }
    }
}

// This is a test for checking memory allocation
fn check_alloc() {
    let long_lived = Box::new(1); // new
    for i in 0..32 {
        let x = Box::new(i);
        println!("{}", i);
        println! {"{:p}" ,ptr::addr_of!(*x)};
        assert_eq!(*x, i);
    }
    assert_eq!(*long_lived, 1); // new
}

fn print_help() {
    println!("help\t: print this help menu");
    println!("hello\t: print Hello World!");
    println!("reboot\t: reboot the device");
    println!("info\t: show the device info");
    println!("alloc\t: test allocator");
    println!("ls\t: list file");
    println!("cat\t: print file");
}
