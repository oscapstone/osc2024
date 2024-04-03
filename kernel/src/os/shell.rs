use super::stdio::{get_line, print};
use super::file_system::cpio;
use crate::println;

pub fn start(initrd_start: u32) {
    let mut inp_buf = [0u8; 256];

    loop {
        print("> ".into());
        let _len = get_line(&mut inp_buf, 256);

        if inp_buf.starts_with(b"help") {
            println!("help\t:print this help menu");
            println!("hello\t:print Hello, World!");
            println!("reboot\t:reboot the device");
            println!("ls\t:list files in the initramfs");
            println!("cat\t:print the content of a file in the initramfs");

        } else if inp_buf.starts_with(b"hello") {
            println!("Hello, World!");

        } else if inp_buf.starts_with(b"reboot") {
            println!("Rebooting...");
            crate::cpu::reboot::reset(100);
            break;

        } else if inp_buf.starts_with(b"ls") {
            let init_ram_file = cpio::CpioArchive::load(initrd_start as *const u8);
            init_ram_file.print_file_list();

        } else if inp_buf.starts_with(b"cat") {
            println!("Filename: ");
            let len = get_line(&mut inp_buf, 256);
            let filename = core::str::from_utf8(&inp_buf[..len - 1]).unwrap();
            let init_ram_file = cpio::CpioArchive::load(initrd_start as *const u8);
            init_ram_file.print_file_content(filename);

        } else if inp_buf.starts_with(b"M3") {
            println!("香的發糕");
        
        } else {
            println!("Unknown command {}", core::str::from_utf8(&inp_buf).unwrap().trim_end());
        }

    }
}