pub mod cpu;
pub mod state;

use alloc::alloc::{alloc, Layout};
use stdio::println;

#[repr(C)]
#[derive(Debug)]
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
        let stack =
            unsafe { alloc(Layout::from_size_align(stack_size, 0x100).unwrap()) as *mut u8 };
        let cpu_state = cpu::State::new(stack, stack_size, entry);
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + stack_size
        );
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
            unsafe { alloc(Layout::from_size_align(self.stack_size, 0x100).unwrap()) as *mut u8 };
        unsafe {
            core::ptr::copy(self.stack, stack, self.stack_size);
        }
        let mut cpu_state = self.cpu_state.clone();
        cpu_state.sp = (cpu_state.sp as usize - self.stack as usize + stack as usize) as u64;
        println!(
            "Cloning thread 0x{:x} to 0x{:x}",
            self.id, 0xdeadbeaf as u32
        );
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + self.stack_size
        );
        Thread {
            id: 0xdeadbeaf,
            stack,
            cpu_state,
            ..*self
        }
    }
}
