pub mod cpu;
pub mod state;

use alloc::alloc::{alloc, Layout};
use stdio::println;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Thread {
    pub cpu_state: cpu::State,
    pub id: u32,
    pub state: state::State,
    pub stack: *mut u8,
    pub stack_size: usize,
    pub entry: extern "C" fn(),
    pub code_range: (u64, u64),
    pub args: *mut u8,
}

impl Thread {
    pub fn new(
        id: u32,
        stack_size: usize,
        entry: extern "C" fn(),
        entry_size: usize,
        args: *mut u8,
    ) -> Self {
        let stack = unsafe { alloc(Layout::from_size_align(stack_size, 16).unwrap()) as *mut u8 };
        let cpu_state = cpu::State::new(stack, stack_size, entry);
        println!(
            "Thread {} stack: 0x{:x} ~ 0x{:x}",
            id,
            stack as u64,
            stack as u64 + stack_size as u64
        );
        Thread {
            id,
            state: state::State::Init,
            stack,
            stack_size,
            entry,
            code_range: (entry as u64, entry as u64 + entry_size as u64),
            args,
            cpu_state,
        }
    }
}
