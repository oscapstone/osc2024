#![no_std]
#![no_main]

use cpio::CpioArchive;
use panic_wait as _;
use small_std::println;

use crate::shell::ShellCommand;

mod boot;
mod driver;
mod shell;

const CPIO_ADDR: usize = 0x800_0000;

unsafe fn kernel_init() -> ! {
    if let Err(e) = driver::register_drivers() {
        panic!("Failed to initialize driver subsystem: {}", e);
    }

    device::driver::driver_manager().init_drivers();

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
    device::driver::driver_manager().enumerate();

    println!("[2] Echoing input now");

    let cpio = unsafe { CpioArchive::new(CPIO_ADDR) };
    let commands: &[&dyn ShellCommand] = &[
        &shell::commands::HelloCommand,
        &shell::commands::RebootCommand,
        &shell::commands::InfoCommand,
        &shell::commands::LsCommand::new(&cpio),
        &shell::commands::CatCommand::new(&cpio),
    ];
    let mut shell = shell::Shell::new(commands);
    shell.run_loop();
}
