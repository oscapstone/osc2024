use core::mem::size_of;

use aarch64_cpu::registers::*;
use alloc::boxed::Box;

use crate::scheduler::{current, task::Task};

#[inline(never)]
pub fn sig_return() {
    let signal_stack_context_addr = SP_EL0.get() as usize + 2 * size_of::<fn() -> !>();
    let signal_stack = signal_stack_context_addr - Task::USER_STACK_SIZE;
    // release the signal stack
    unsafe {
        let _ = Box::from_raw(signal_stack as *mut [u8; Task::USER_STACK_SIZE]);
    };
    // restore the context
    (unsafe { &mut *current() }).load_signal_context();
}
