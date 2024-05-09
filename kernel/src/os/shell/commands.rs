use crate::println;

pub fn help() {
    println!("help\t:print this help menu");
    println!("hello\t:print Hello, World!");
    println!("reboot\t:reboot the device");
    println!("ls\t:list files in the initramfs");
    println!("cat\t:print the content of a file in the initramfs");
    println!("exec\t:load a file to memory and execute it");
}