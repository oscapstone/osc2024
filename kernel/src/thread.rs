pub mod cpu;
pub mod state;

use alloc::alloc::{alloc, Layout};

#[repr(C)]
#[derive(Copy, Debug)]
pub struct Thread {
    pub id: usize,
    pub state: state::State,
    pub stack: *mut u8,
    pub stack_size: usize,
    pub entry: extern "C" fn(),
    pub cpu_state: cpu::State,
}

impl Thread {
    pub fn new(stack_size: usize, entry: extern "C" fn()) -> Self {
        let stack = unsafe { alloc(Layout::from_size_align(stack_size, 16).unwrap()) as *mut u8 };
        let cpu_state = cpu::State::new(stack, stack_size, entry);
        Thread {
            id: 0xC8763,
            state: state::State::Ready,
            stack,
            stack_size,
            entry,
            cpu_state,
        }
    }
}

impl Clone for Thread {
    fn clone(&self) -> Self {
        let stack =
            unsafe { alloc(Layout::from_size_align(self.stack_size, 16).unwrap()) as *mut u8 };
        let mut cpu_state = self.cpu_state.clone();
        cpu_state.sp = (cpu_state.sp as usize - self.stack as usize + stack as usize) as u64;
        Thread {
            id: self.id,
            state: self.state,
            stack,
            stack_size: self.stack_size,
            entry: self.entry,
            cpu_state,
        }
    }
}
