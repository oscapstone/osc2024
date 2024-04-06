use core::arch::asm;
use core::sync::atomic::{AtomicBool, Ordering};

/// Stop immediately if called a second time.
pub fn panic_prevent_reenter() {
    // Using atoms can save us from needing `unsafe` here.

    static PANIC_IN_PROGRESS: AtomicBool = AtomicBool::new(false);

    if !PANIC_IN_PROGRESS.load(Ordering::Relaxed) {
        PANIC_IN_PROGRESS.store(true, Ordering::Relaxed);
        return;
    }

    wait_forever();
}

#[inline(always)]
pub fn wait_forever() -> ! {
    loop {
        // SAFETY: This is a safe operation.
        unsafe { asm!("wfe") };
    }
}
