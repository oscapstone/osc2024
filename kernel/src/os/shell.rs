use super::file_system::cpio;
use super::stdio::{get_line, print};
use crate::os::stdio::println_now;
use crate::os::timer;
use crate::println;
use core::arch::asm;
use core::time;
use alloc::string::{String, ToString};
use alloc::boxed::Box;
mod commands;

fn print_time(time: u64) {
    let sec = time / 1000;
    let ms = time % 1000;
    println!("Time: {}.{:03} s", sec, ms);
    // set_next_timer(2000);
}

fn set_next_timer() {
    // println!("Timer expired");
    timer::add_timer_ms(2000, Box::new(|| set_next_timer()));
    // for c in b"Timer expired -- Function\n" {
    //     unsafe {
    //         crate::cpu::uart::send(*c);
    //     }
    // }
    // loop {}
}

pub fn start(initrd_start: u32) {
    let mut inp_buf = [0u8; 256];

    loop {
        print("> ".into());
        let len = get_line(&mut inp_buf, 256);
        let initramfs = cpio::CpioArchive::load(initrd_start as *const u8);

        if inp_buf.starts_with(b"help") {
            commands::help();
        } else if inp_buf.starts_with(b"hello") {
            println!("Hello, World!");
        } else if inp_buf.starts_with(b"reboot") {
            println!("Rebooting...");
            crate::cpu::reboot::reset(100);
            break;
        } else if inp_buf.starts_with(b"ls") {
            for i in initramfs.get_file_list() {
                println!(i);
            }
        } else if inp_buf.starts_with(b"cat") {
            print!("Filename: ");
            let len = get_line(&mut inp_buf, 256);
            let filename = core::str::from_utf8(&inp_buf[..len - 1]).unwrap();
            initramfs.print_file_content(filename);
        } else if inp_buf.starts_with(b"exec") {
            print!("Filename: ");
            let len = get_line(&mut inp_buf, 256);
            let filename = core::str::from_utf8(&inp_buf[..len - 1]).unwrap();
            match initramfs.load_file_to_memory(filename, 0x2001_0000 as *mut u8) {
                true => println!("File loaded to memory"),
                false => {
                    println!("File not found");
                    continue;
                }
            }

            timer::add_timer_ms(2000, Box::new(|| set_next_timer()));

            unsafe {
                asm!(
                    "mov {tmp}, 0x200",
                    "msr spsr_el1, {tmp}",
                    "ldr {tmp}, =0x20010000", // Program counter
                    "msr elr_el1, {tmp}",
                    "ldr {tmp}, =0x2000F000", // Stack pointer
                    "msr sp_el0, {tmp}",
                    "eret",
                    tmp = out(reg) _,
                );

                println!("Should not reach here");
            }
        } else if inp_buf.starts_with(b"M3") {
            println!("香的發糕");

        } else if inp_buf.starts_with(b"setTimeout") {
            let inp_buf = &inp_buf[..len - 1];
            let mut iter = inp_buf.split(|c| c.clone() == b' ');
            assert_eq!(iter.next().unwrap(), b"setTimeout");
            let message = core::str::from_utf8(iter.next().unwrap()).unwrap().to_string();
            let time = core::str::from_utf8(iter.next().unwrap()).unwrap();
            match time.parse::<u64>() {
                Ok(time) => {
                    timer::add_timer_ms(time, move || println!("{}", message));
                }
                Err(_) => {
                    println!("Invalid time: {}", time.len());
                }
            }
            
            // timer::add_timer_ms(time, move || println!("{}", message));
        
        } else {
            println!(
                "Unknown command {}",
                core::str::from_utf8(&inp_buf).unwrap().trim_end()
            );
        }
    }
}
