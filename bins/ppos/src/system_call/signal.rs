use crate::{scheduler::current, signal::Signal};

pub fn signal(signal: i32, handler: fn() -> !) {
    let current = unsafe { &mut *current() };
    current.set_signal_handler(Signal::from(signal as usize), handler)
}
