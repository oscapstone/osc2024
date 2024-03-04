#![feature(asm_const)]
#![no_main]
#![no_std]

use aarch64_cpu::asm;
use core::arch::global_asm;
use core::panic::PanicInfo;

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
    crate::kernel_init()
}

/// Early kernel initialization.
/// # Safety
///
/// - Only a single core must be active and running this function.
unsafe fn kernel_init() -> ! {
    panic!("only one core is supported");
}

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

global_asm!(include_str!("boot.S"),
CONST_CORE_ID_MASK = const 0b11
);
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    wait_forever()
}

#[inline(always)]
pub fn wait_forever() -> ! {
    loop {
        asm::wfe()
    }
}
