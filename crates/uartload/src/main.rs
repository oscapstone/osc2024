#![no_std]
#![no_main]

mod drivers;

use core::arch::global_asm;
use panic_wait as _;
use small_std::println;

const BANNER: &str = r#"
  __  _____   ___  ________   ____  ___   ___ 
 / / / / _ | / _ \/_  __/ /  / __ \/ _ | / _ \
/ /_/ / __ |/ , _/ / / / /__/ /_/ / __ |/ // /
\____/_/ |_/_/|_| /_/ /____/\____/_/ |_/____/ 
                                              "#;

global_asm!(include_str!("boot.s"));

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    if let Err(e) = drivers::register_drivers() {
        panic!("Failed to initialize driver subsystem: {}", e);
    }

    device::driver::driver_manager().init_drivers();

    main();
}

fn main() -> ! {
    println!("{}", BANNER);

    loop {}
}
