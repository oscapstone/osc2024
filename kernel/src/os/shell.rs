use super::stdio::{get_line, print, println};

pub fn start() {
    let mut inp_buf = [0u8; 256];

    loop {
        print("> ".into());
        let len = get_line(&mut inp_buf, 256);

        if inp_buf.starts_with(b"help") {
            println("help\t:print this help menu".into());
            println("hello\t:print Hello, World!".into());
            println("reboot\t:reboot the device".into());
        } else if inp_buf.starts_with(b"hello") {
            println("Hello, World!".into());
        } else if inp_buf.starts_with(b"reboot") {
            println("Rebooting...".into());
            crate::cpu::reboot::reset(100);
            break;
        }
    }
}