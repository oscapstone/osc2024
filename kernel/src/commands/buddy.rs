use alloc::vec::Vec;
use stdio::gets;
use stdio::print;
use stdio::println;

use crate::allocator::buddy::BUDDY_SYSTEM;

const MAX_COMMAND_LEN: usize = 0x100;

pub fn exec() {
    let mut buf: [u8; MAX_COMMAND_LEN] = [0; MAX_COMMAND_LEN];
    loop {
        print!("buddy# ");
        gets(&mut buf);
        if execute_command(&buf) {
            break;
        }
    }
}

fn execute_command(buf: &[u8]) -> bool {
    let buf = match buf.iter().position(|&c| c == 0) {
        Some(i) => &buf[..i],
        None => buf,
    };
    let args = match core::str::from_utf8(&buf) {
        Ok(s) => s.split_whitespace().collect::<Vec<&str>>(),
        Err(_) => {
            println!("Invalid arguments");
            return false;
        }
    };
    if args.len() == 0 {
        return false;
    }
    println!("Args: {:?}", args);
    match args[0] {
        "info" => unsafe {
            BUDDY_SYSTEM.print_info();
        },
        "verbose" => unsafe {
            BUDDY_SYSTEM.toggle_verbose();
        },
        "reserve" => unsafe {
            let idx = match args.get(1) {
                Some(s) => match s.parse::<usize>() {
                    Ok(n) => n,
                    Err(_) => {
                        println!("Invalid frame index");
                        return false;
                    }
                },
                None => {
                    println!("Please specify frame index");
                    return false;
                }
            };
            println!("Reserving frame {}", idx);
            if BUDDY_SYSTEM.reserve_frame(idx) {
                println!("Frame {} is reserved", idx);
            } else {
                println!("Reserving frame {} failed", idx);
            }
        },
        "free" => unsafe {
            let idx = match args.get(1) {
                Some(s) => match s.parse::<usize>() {
                    Ok(n) => n,
                    Err(_) => {
                        println!("Invalid frame index");
                        return false;
                    }
                },
                None => {
                    println!("Please specify frame index");
                    return false;
                }
            };
            println!("Freeing frame {}", idx);
            println!("WARNING: You should only free a frame that you have reserved before!");
            BUDDY_SYSTEM.free_by_idx(idx, 0);
            println!("Frame {} is freed", idx);
        },
        "exit" => return true,
        _ => {
            println!("Unknown command: {}", args[0]);
        }
    }
    false
}
