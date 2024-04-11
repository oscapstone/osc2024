#![feature(asm_const)]
#![no_std]
#![no_main]

mod drivers;

use core::arch::{asm, global_asm};
use panic_wait as _;
use small_std::{fmt::print::console::console, println};

const RPI3_DEFAULT_LOAD_ADDR: *mut u8 = 0x80000 as *mut u8;
static mut DEVICETREE_START_ADDR: usize = 0;

const BANNER: &str = r#"
  __  _____   ___  ________   ____  ___   ___ 
 / / / / _ | / _ \/_  __/ /  / __ \/ _ | / _ \
/ /_/ / __ |/ , _/ / / / /__/ /_/ / __ |/ // /
\____/_/ |_/_/|_| /_/ /____/\____/_/ |_/____/ 
                                              "#;

global_asm!(
    include_str!("boot.s"),
    CONST_CORE_ID_MASK= const 0b11,
);

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    asm!("mov {}, x0", out(reg) DEVICETREE_START_ADDR);

    if let Err(e) = drivers::register_drivers() {
        panic!("Failed to initialize driver subsystem: {}", e);
    }

    device::driver::driver_manager().init_drivers();

    main();
}

fn main() -> ! {
    println!("{}", BANNER);

    println!("[uartload] startup complete.");
    println!("[uartload] waiting for kernel sent through UART");

    // Make sure the console is in a clean state
    console().flush();
    console().clear_rx();

    let image_size = {
        let mut num = 0;
        for i in 0..4 {
            let byte = console().read_char() as u32;
            num |= byte << (i * 8);
        }
        num
    };

    console().write_char('O');
    console().write_char('K');
    console().flush();

    unsafe {
        for i in 0..image_size {
            let byte = console().read_char() as u8;
            RPI3_DEFAULT_LOAD_ADDR
                .offset(i as isize)
                .write_volatile(byte);
        }
    }

    println!("[uartload] kernel received, jumping to kernel");
    console().flush();

    unsafe {
        let kernel: fn(usize) -> ! = core::mem::transmute(RPI3_DEFAULT_LOAD_ADDR);
        kernel(DEVICETREE_START_ADDR);
    }
}
