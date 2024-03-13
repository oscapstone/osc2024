use core::arch::global_asm;
global_asm!(include_str!("boot.s"));

/// The Rust entry of the `kernel` binary.
///
/// The function is called from the assembly `_start` function.
#[no_mangle]
pub fn _start_rust() -> ! {
    crate::kernel::init();
}
