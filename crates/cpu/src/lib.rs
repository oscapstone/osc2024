#![no_std]

use aarch64_cpu::asm;

/// Spin for `n` cycles
#[inline(always)]
pub fn spin_for_cycles(n: usize) {
    for _ in 0..n {
        asm::nop();
    }
}

/// Pause the execution of the core.
#[inline(always)]
pub fn wait_forever() -> ! {
    loop {
        asm::wfe()
    }
}
