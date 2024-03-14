#![feature(asm_const)]
#![feature(panic_info_message)]
#![feature(trait_alias)]
#![no_main]
#![no_std]

use core::arch::global_asm;

mod bcm;
mod console;
mod panic;
mod print;
mod synchronization;
mod shell;

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

/// The Rust entry of the `kernel` binary.
///
/// The function is called from the assembly `_start` function.
///
/// # Safety
///
/// 1. boot.S link start entry to _start
/// 2. only core 0 would init bss, sp... and call _start_rust
/// 3. _start_rust call kernel_init
#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    bcm::hardware_init();
    crate::kernel_init();
}


/// Early kernel initialization.
/// # Safety
///
/// - Only a single core must be active and running this function.
unsafe fn kernel_init() -> ! {
    kernel_main()
}

unsafe fn kernel_main() -> ! {
    println!("[0] Hello from Rust!");
    println!("[1] run the simple shell");
    shell::interactiave_shell()
}

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

global_asm!(include_str!("boot.S"),
CONST_CORE_ID_MASK = const 0b11
);
