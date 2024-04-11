//! A panic handler that infinitely waits.

#![feature(panic_info_message)]
#![no_std]

mod utils;
use core::panic::PanicInfo;
use small_std::println;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    utils::panic_prevent_reenter();

    let (location, line, column) = match info.location() {
        Some(location) => (location.file(), location.line(), location.column()),
        None => ("<unknown>", 0, 0),
    };

    println!(
        "Kernel panicked!\n\n\
        Panic location: {}:{}:{}\n\n\
        {}",
        location,
        line,
        column,
        info.message().unwrap()
    );

    utils::wait_forever();
}
