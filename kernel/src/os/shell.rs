use super::file_system::cpio;
use super::stdio::{get_line, print, print_hex_now, println_now};
use crate::cpu::uart::recv_async;
use crate::os::timer;
use crate::println;
use alloc::boxed::Box;
use alloc::string::{String, ToString};
use core::arch::asm;
mod commands;
use alloc::vec::Vec;

fn print_time(time: u64) {
    let sec = time / 1000;
    let ms = time % 1000;
    println!("Time: {}.{:03} s", sec, ms);
    // set_next_timer(2000);
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

fn get_input(commands: &Vec<commands::command>) -> String {
    let mut input_buffer = String::new();

    loop {
        if let Some(c) = unsafe { recv_async() } {
            if c == b'\r' {
                println!("");
                break input_buffer;
            } else if c == b'\t' {
                let mut matched_commands = Vec::new();
                for command in commands.iter() {
                    let name = command.get_name();
                    if name.starts_with(input_buffer.as_str()) {
                        matched_commands.push(command);
                    }
                }

                if matched_commands.len() == 1 {
                    let match_name = matched_commands[0].get_name();
                    let match_name_remaining = &match_name[input_buffer.len()..];
                    input_buffer.push_str(match_name_remaining);
                    print!("{}", match_name_remaining);
                } else {
                    if matched_commands.len() > 1 {
                        // find the common prefix
                        'prefix: for idx in input_buffer.len().. {
                            match matched_commands[0].get_name().as_bytes().get(idx) {
                                Some(c) => {
                                    for cmd in &matched_commands {
                                        if cmd.get_name().as_bytes().get(idx).unwrap_or(&0).clone()
                                            != *c
                                        {
                                            break 'prefix;
                                        }
                                    }
                                    input_buffer.push(*c as char);
                                }
                                None => break,
                            }
                        }
                    }

                    let padding = commands.iter().map(|cmd| cmd.get_name().len()).max().unwrap_or(0);

                    // print all matches
                    println!("");
                    for cmd in matched_commands.iter() {
                        println!("{: <width$}: {}", cmd.get_name(), cmd.get_description(), width = padding);
                    }
                    print!("> ");
                    print!("{}", input_buffer);
                }
            } else {
                input_buffer.push(c as char);
                print!("{}", c as char);
            }
        }
    }
}

pub fn start(initrd_start: u32) {
    unsafe {
        INITRAMFS = Some(cpio::CpioArchive::load(initrd_start as *const u8));
    }

    let mut commands = Vec::new();
    push_all_commands(&mut commands);
    commands.sort_by(|a, b| a.get_name().cmp(b.get_name()));

    'shell: loop {
        print!("> ");

        let input = get_input(&commands);
        let mut input = input.trim().split_whitespace();

        let input_command = input.next().unwrap_or("");
        let input_args: Vec<&str> = input.collect();
        let input_args = {
            let mut args = Vec::new();
            for arg in input_args.iter() {
                args.push(String::from(*arg));
            }
            args
        };

        if input_command == "help" {
            let padding = commands.iter().map(|cmd| cmd.get_name().len()).max().unwrap_or(0);
            for cmd in commands.iter() {
                println!("{: <width$}: {}", cmd.get_name(), cmd.get_description(), width = padding);
            }
            continue 'shell;
        }

        for command in commands.iter() {
            if input_command == command.get_name() {
                command.execute(input_args);
                continue 'shell;
            }
        }

        println!("Command not found");
    }
}
