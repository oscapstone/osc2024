use crate::scheduler::current;

#[inline(always)]
pub fn sig_return() {
    unsafe { &mut *current() }.back_from_signal();
}
