use crate::{
    bcm::{self, common, mailbox::MailboxTag, UART},
    console::interface::Statistics,
    memory, print, println,
};

const MAXCHAR: usize = 100;

fn reboot() {
    println!("Rebooting...");
    common::reset(100);
}

fn help() {
    println!("help    : print this help menu");
    println!("hello   : print Hello World!");
    println!("board   : print board rev");
    println!("status  : print UART status");
    println!("reboot  : reboot this device");
    println!("ls      : list initramfs files");
    println!("cat     : print file content");
    println!("mem     : check memory allocation");
    println!("cancel  : cancel reboot");
}

pub fn interactiave_shell() -> ! {
    let mut array: [u8; MAXCHAR] = [0; MAXCHAR];
    let mut cnt = 0;

    // parse fdt tree
    memory::device_tree::fdt_traverse(memory::device_tree::set_initrd_start);

    loop {
        let c = bcm::UART.get_char();
        array[cnt] = c;
        let c = c as char;
        print!("{}", c);
        if c == '\r' {
            println!();
            // println!("{}", core::str::from_utf8(&array[0..cnt]).unwrap());
            let cmd = core::str::from_utf8(&array[0..cnt]).unwrap_or(" ");
            let cmd_0 = cmd.split_whitespace().next().unwrap_or("");
            let arg_1 = cmd.split_whitespace().nth(1).unwrap_or("");
            match cmd_0 {
                "help" => {
                    help();
                }
                "hello" => {
                    println!("Hello World!");
                }
                "reboot" => {
                    println!("Rebooting...");
                    reboot();
                }
                "cancel" => {
                    common::cancel_reset();
                }
                "board" => {
                    let (board, _) = bcm::MAILBOX.get(MailboxTag::BoardRevision);
                    let (mem0, mem1) = bcm::MAILBOX.get(MailboxTag::ArmMemory);
                    let (serial0, serial1) = bcm::MAILBOX.get(MailboxTag::BoardSerial);
                    let (vc0, vc1) = bcm::MAILBOX.get(MailboxTag::VcMemory);
                    println!("Board revision: {:x}", board);
                    println!("Board serial: {:x} {:x}", serial0, serial1);
                    println!("Arm memory(base, size): {:x}, {:x}", mem0, mem1);
                    println!("Vc memory(base, size):  {:x}, {:x}", vc0, vc1);
                }
                "status" => {
                    println!("Chars written: {}", UART.chars_written());
                }
                "ls" => {
                    // memory::list_initramfs_files();
                    memory::initramfs::list_initramfs_files();
                }
                "cat" => {
                    // memory::get_initramfs_files(arg_1);
                    memory::initramfs::get_initramfs_files(arg_1);
                }
                "mem" => {
                    println!("Initrd start: {:x}", memory::device_tree::get_initrd_start());
                }
                _ => {
                    if cnt > 0 {
                        println!(
                            "Unknown command: {:?}",
                            &core::str::from_utf8(&array[0..cnt]).unwrap_or("")
                        );
                        help();
                    }
                }
            }

            print!("\r# ");
            cnt = 0;
        }
        cnt += if c == '\r' { 0 } else { 1 };
        cnt = cnt % MAXCHAR;
    }
}
