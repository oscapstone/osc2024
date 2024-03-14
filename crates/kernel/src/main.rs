#![no_std]
#![no_main]

use panic_wait as _;
use small_std::println;

mod boot;
mod driver;
mod shell;

unsafe fn kernel_init() -> ! {
    if let Err(e) = driver::register_drivers() {
        panic!("Failed to initialize driver subsystem: {}", e);
    }

    device::device_driver::driver_manager().init_drivers();

    // Finnaly go from unsafe to safe 🎉
    main()
}

fn main() -> ! {
    println!(
        "[0] {} version {}",
        env!("CARGO_PKG_NAME"),
        env!("CARGO_PKG_VERSION")
    );

    println!("[1] Drivers loaded:");
    device::device_driver::driver_manager().enumerate();

    println!("[2] Echoing input now");

    let mut shell = shell::Shell::new();
    shell.run_loop();
}
