mod buddy;
mod cat;
mod echo;
mod exec;
mod hello;
mod help;
mod ls;
mod reboot;
mod set_time_out;
use stdio::println;

use alloc::string::String;
use alloc::string::ToString;
use alloc::vec::Vec;

pub fn execute(command: &[u8]) {
    let command = match command.iter().position(|&c| c == 0) {
        Some(i) => &command[..i],
        None => command,
    };
    let args: Vec<String> = core::str::from_utf8(command)
        .unwrap()
        .split_whitespace()
        .map(|s| s.to_string())
        .collect();
    println!("Executing command: {:?}", args);
    if args.is_empty() {
        return;
    } else if args[0] == "hello" {
        hello::exec();
    } else if args[0] == "help" {
        help::exec();
    } else if args[0] == "reboot" {
        reboot::exec();
    } else if args[0] == "ls" {
        ls::exec();
    } else if args[0] == "cat" {
        cat::exec(&command);
    } else if args[0] == "exec" {
        exec::exec(args);
    } else if args[0] == "echo" {
        echo::exec(&command);
    } else if args[0] == "setTimeOut" {
        set_time_out::exec(&command);
    } else if args[0] == "buddy" {
        buddy::exec();
    } else {
        println!(
            "Unknown command: {}",
            core::str::from_utf8(command).unwrap()
        );
    }
}
