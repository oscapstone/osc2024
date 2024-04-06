use core::arch::asm;

#[inline(always)]
pub fn spin_for_cycles(n: usize) {
    for _ in 0..n {
        // SAFETY: This is a safe operation.
        unsafe { asm!("nop") };
    }
}
