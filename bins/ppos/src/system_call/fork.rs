use core::arch::asm;

use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};

use crate::scheduler::current;

pub fn fork() -> i32 {
    unsafe { disable_kernel_space_interrupt() }
    let mut sp;
    unsafe {
        asm!(
            "mov {}, sp",
            out(reg) sp,
        );
    }
    let child = unsafe { &*current() }.fork(sp);
    unsafe { enable_kernel_space_interrupt() }
    if current() == child {
        0
    } else {
        (unsafe { &*child }).pid_number() as i32
    }
}
