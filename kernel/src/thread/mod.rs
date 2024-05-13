pub mod cpu;
pub mod state;

use crate::scheduler::{get_current, thread_wrapper};
use core::arch::{asm, global_asm};
use stdio::println;

global_asm!(include_str!("context_switch.S"));

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Thread {
    pub cpu_state: cpu::State,
    pub id: u32,
    pub state: state::State,
    pub stack: *mut u8,
    pub stack_size: usize,
    pub entry: extern "C" fn(*mut u8),
    pub args: *mut u8,
}

impl Thread {
    pub fn new(id: u32, stack_size: usize, entry: extern "C" fn(*mut u8), args: *mut u8) -> Self {
        let stack = unsafe {
            alloc::alloc::alloc(alloc::alloc::Layout::from_size_align(stack_size, 16).unwrap())
                as *mut u8
        };
        let cpu_state = cpu::State::new(stack, stack_size, thread_wrapper);
        println!("Thread {} stack: 0x{:x}", id, stack as u64);
        Thread {
            id,
            state: state::State::Init,
            stack,
            stack_size,
            entry,
            args,
            cpu_state,
        }
    }
}

pub extern "C" fn test_func(args: *mut u8) {
    let args = unsafe { &*(args as *const u32) };
    let tid = unsafe { (&*get_current()).id };
    for i in 0..10_000 {
        let sp: u64;
        unsafe { asm!("mov {}, sp", out(reg) sp) };
        println!("Thread[{}]: tid={}, {}, sp=0x{:x}", args, tid, i, sp);
        let tm = crate::timer::manager::get();
        // tm.print();
        for _ in 0..tm.get_frequency() / 2 {
            unsafe {
                asm!("nop");
            }
        }
    }
}
