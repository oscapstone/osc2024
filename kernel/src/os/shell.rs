use super::file_system::cpio;
use super::stdio::{get_line, print, print_hex_now, println_now};
use crate::cpu::uart::recv_async;
use crate::os::timer;
use crate::println;
use alloc::boxed::Box;
use alloc::string::ToString;
use core::arch::asm;
mod commands;
use alloc::vec::Vec;

fn print_time(time: u64) {
    let sec = time / 1000;
    let ms = time % 1000;
    println!("Time: {}.{:03} s", sec, ms);
    // set_next_timer(2000);
}

fn clear_buffer(buf: &mut [u8]) {
    for c in buf.iter_mut() {
        *c = 0;
    }
}

static mut INITRAMFS: Option<cpio::CpioArchive> = None;

fn push_all_commands(commands: &mut Vec<commands::command>) {
    commands.push(commands::command::new(
        "help",
        "Print help message",
        commands::help,
    ));
    commands.push(commands::command::new(
        "hello",
        "print Hello World",
        commands::hello,
    ));
    commands.push(commands::command::new(
        "reboot",
        "Reboot the device",
        commands::reboot,
    ));
    commands.push(commands::command::new(
        "ls",
        "List files in the initramfs",
        commands::ls,
    ));
    commands.push(commands::command::new(
        "cat",
        "Print the content of a file in the initramfs",
        commands::cat,
    ));
    commands.push(commands::command::new(
        "exec",
        "Execute a program in the initramfs",
        commands::exec,
    ));
    commands.push(commands::command::new(
        "setTimeout",
        "Set a timer to print a message after a certain time",
        commands::set_timeout,
    ));
    commands.push(commands::command::new(
        "test_memory",
        "Test buddy allocator",
        commands::test_memory,
    ));
}

pub fn start(initrd_start: u32) {
    let mut inp_buf = [0u8; 256];
    unsafe {
        INITRAMFS = Some(cpio::CpioArchive::load(initrd_start as *const u8));
    }

    let mut commands = Vec::new();
    push_all_commands(&mut commands);

    'shell: loop {
        print!("> ");
        clear_buffer(&mut inp_buf[..]);

        let mut len = 0;

        loop {
            if let Some(c) = unsafe { recv_async() } {
                if c == b'\r' {
                    println!("");
                    break;
                } else if c == b'\t' {
                    let mut matches = Vec::new();
                    for command in commands.iter() {
                        let name = command.get_name();
                        let input = core::str::from_utf8(&inp_buf[..len]).unwrap();
                        if name.starts_with(input) {
                            matches.push(command);
                        }
                    }

                    if matches.len() == 1 {
                        let match_name = matches[0].get_name();
                        let match_name_bytes = match_name.as_bytes();
                        for i in len..match_name_bytes.len() {
                            inp_buf[len] = match_name_bytes[i];
                            len += 1;
                            print!("{}", match_name_bytes[i] as char);
                        }
                    } else {
                        if matches.len() > 1 {
                            // find the common prefix
                            let mut prefix = Vec::new();
                            let mut idx = 0;
                            'prefix: loop {
                                let c = matches[0].get_name().as_bytes()[idx];
                                for cmd in matches.iter() {
                                    if cmd.get_name().as_bytes()[idx] != c {
                                        break 'prefix;
                                    }
                                }
                                prefix.push(c);
                                idx += 1;
                            }
                            let ori_len = len;
                            for c in prefix[len..].iter() {
                                inp_buf[len] = *c;
                                len += 1;
                            }
                        }

                        // print all matches
                        println!("");
                        for cmd in matches.iter() {
                            println!("{}\t:{}", cmd.get_name(), cmd.get_description());
                        }
                        print!("> ");
                        for i in 0..len {
                            print!("{}", inp_buf[i] as char);
                        }
                    }
                } else {
                    inp_buf[len] = c;
                    len += 1;
                    print!("{}", c as char);
                }
            }
        }
        
        if !inp_buf[0].is_ascii_alphabetic() {
            continue;
        }

        for command in commands.iter() {
            if inp_buf.starts_with(command.get_name().as_bytes()) {
                command.execute(Vec::new());
                continue 'shell;
            }
        }

        println!("Command not found");
    }
}
