use core::arch::asm;

/// Wait for event. A simple safe wrapper for the `wfe` instruction.
#[inline(always)]
pub fn wfe() {
    // SAFETY: there's no memory operation
    unsafe { asm!("wfe") };
}
