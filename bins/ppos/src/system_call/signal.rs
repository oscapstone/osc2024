use crate::{scheduler::current, signal::Signal};

pub fn signal(signal: i32, handler: fn() -> !) {
    unsafe { &mut *current() }.set_signal_handler(Signal::from(signal as usize), handler)
}
