use aarch64_cpu::asm;

// public code
/// pause execution on the core

#[inline(always)]
pub fn wait_forever() -> ! {
    loop {
        asm::wfe();
    }
}
