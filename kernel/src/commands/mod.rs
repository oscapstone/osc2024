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
    let args: Vec<String> = core::str::from_utf8(command)
        .unwrap()
        .split_whitespace()
        .map(|s| s.to_string())
        .collect();

    if command.starts_with(b"\x00") {
        return;
    } else if command.starts_with(b"hello") {
        hello::exec();
    } else if command.starts_with(b"help") {
        help::exec();
    } else if command.starts_with(b"reboot") {
        reboot::exec();
    } else if command.starts_with(b"ls") {
        ls::exec();
    } else if command.starts_with(b"cat") {
        cat::exec(&command);
    } else if command.starts_with(b"exec") {
        exec::exec(args);
    } else if command.starts_with(b"echo") {
        echo::exec(&command);
    } else if command.starts_with(b"setTimeOut") {
        set_time_out::exec(&command);
    } else if command.starts_with(b"buddy") {
        buddy::exec();
    } else {
        println!(
            "Unknown command: {}",
            core::str::from_utf8(command).unwrap()
        );
    }
}
