#![feature(asm_const)]
#![feature(panic_info_message)]
#![feature(trait_alias)]
#![no_main]
#![no_std]

use core::arch::global_asm;

use rust_kernel::bcm;
use rust_kernel::println;

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
    println!("[0] Start receiving kernel...");
    let mut size : u32 = 0;
    for _ in 0..4 {
        // Receive the kernel size(4 bytes, little endian) from UART
        size = size << 8;
        let byte = bcm::UART.get_char();
        size = size | (byte as u32);
    }
    println!("[1] Kernel size: {}, start receiving payload...", size);
    unsafe {
        let kernel = core::slice::from_raw_parts_mut(0x80000 as *mut u8, size as usize);
        for i in 0..size {
            kernel[i as usize] = bcm::UART.get_char();
        }
    }
    println!("[2] Kernel received, jump to kernel...");
    let kernel = 0x80000 as *const ();
    let kernel: fn() -> ! = core::mem::transmute(kernel);
    kernel();
}

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

global_asm!(include_str!("boot.S"),
CONST_CORE_ID_MASK = const 0b11
);
