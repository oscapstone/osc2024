#![feature(asm_const)]
#![feature(format_args_nl)]
#![feature(panic_info_message)]
#![feature(trait_alias)]
#![no_main]
#![no_std]

mod arrsting;
mod bsp;
mod console;
mod cpu;
mod device_tree;
mod driver;
mod fs;
mod mbox;
mod memory;
mod panic_wait;
mod power;
mod print;
mod shell;
mod synchronization;

use crate::bsp::memory::map::sdram;

extern "C" {
    static __dtb_address: u64;
}
/// Early init code.
///
/// # Safety
///
/// - Only a single core must be active and running this function.
/// - The init calls in this function must appear in the correct order.
unsafe fn kernel_init() -> ! {
    // Initialize the BSP driver subsystem.
    if let Err(x) = bsp::driver::init() {
        panic!("Error initializing BSP driver subsystem: {}", x);
    }

    // Initialize all device drivers.
    driver::driver_manager().init_drivers();
    // println! is usable from here on.

    // Initialize real memory
    memory::ALLOCATOR.init(sdram::RAM_START, sdram::RAM_END);

    // let dtb_addr = 0x50000 as *const u8;
    // let dtb_addr = unsafe { core::ptr::read_volatile(dtb_addr as *const u32) };

    // loop{
    //     println!("dtb_addr: {:#x}", dtb_addr);
    //     println!("dtb_addr_padding {:#x}",  core::ptr::read_volatile(__dtb_address as *const u32));
    // }

    // Init device tree
    let initrd_address = device_tree::get_initrd_start().unwrap();
    println!("initrd_address: {:#x}", initrd_address);

    // Transition from unsafe to safe.
    kernel_main()
}

/// The main function running after the early init.
fn kernel_main() -> ! {
    use console::console;

    println!(
        "[0] {} version {}",
        env!("CARGO_PKG_NAME"),
        env!("CARGO_PKG_VERSION")
    );
    println!("[1] Booting on: {}", bsp::board_name());

    println!("[2] Drivers loaded:");
    driver::driver_manager().enumerate();

    println!("[3] Chars written: {}", console().chars_written());
    println!("[4] Echoing input now");

    shell::start_shell();
}
