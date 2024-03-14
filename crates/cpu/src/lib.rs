#![no_std]

use aarch64_cpu::asm;

/// Pause the execution of the core.
#[inline(always)]
pub fn wait_forever() -> ! {
    loop {
        asm::wfe()
    }
}
