#![no_std]

pub mod asm;

/// Pause the execution of the core.
#[inline(always)]
pub fn wait_forever() -> ! {
    loop {
        asm::wfe()
    }
}
