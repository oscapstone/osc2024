use core::arch::{asm, global_asm};

use aarch64_cpu::{
    asm::barrier,
    registers::{Writeable, ELR_EL1, SPSR_EL1, SP_EL0, TPIDR_EL0, TPIDR_EL1, TTBR0_EL1},
};
use alloc::rc::Rc;
use tock_registers::interfaces::Readable;

use library::sync::mutex::Mutex;

use self::task::Task;

pub mod round_robin_scheduler;
pub mod task;

global_asm!(include_str!("scheduler/cpu_switch_to.s"));

extern "C" {
    fn cpu_switch_to(prev: *mut Task, next: *mut Task) -> *mut Task;
}

unsafe fn software_thread_switch(prev: *mut Task, next: *mut Task) {
    let prev = &mut *prev;
    prev.thread.software_thread_registers.tpidr_el1 = TPIDR_EL1.get();
    prev.thread.software_thread_registers.tpidr_el0 = TPIDR_EL0.get();
    prev.thread.elr_el1 = ELR_EL1.get();
    prev.thread.sp_el0 = SP_EL0.get();
    prev.thread.spsr_el1 = SPSR_EL1.get();

    TPIDR_EL0.set(next as u64);
    TPIDR_EL1.set(next as u64);
    let next = &*next;
    ELR_EL1.set(next.thread.elr_el1);
    SP_EL0.set(next.thread.sp_el0);
    SPSR_EL1.set(next.thread.spsr_el1);

    let baddr = next.memory_mapping().page_table_phys_base_address();
    barrier::dsb(barrier::ISH);
    TTBR0_EL1.set_baddr(baddr);
    asm!("tlbi vmalle1is");
    barrier::dsb(barrier::ISH);
    barrier::isb(barrier::SY);
}

pub unsafe fn switch_to(prev: *mut Task, next: *mut Task) -> *mut Task {
    software_thread_switch(prev, next);
    cpu_switch_to(prev, next)
}

#[inline(always)]
pub fn current() -> *mut Task {
    aarch64_cpu::registers::TPIDR_EL1.get() as *mut Task
}

pub trait Scheduler {
    /// Schedule the next task to run when the timer interrupt occurs.
    fn schedule(&self) -> *mut Task;

    /// Add a task to the scheduler.
    fn add_task(&self, task: Rc<Mutex<Task>>);

    /// Start the scheduler main loop.
    fn start_scheduler(&self) -> !;

    /// Use to check if the scheduler is initialized.
    fn initialized(&self) -> bool;
}

struct NullScheduler {}

impl NullScheduler {
    pub const fn new() -> Self {
        Self {}
    }
}

impl Scheduler for NullScheduler {
    fn schedule(&self) -> *mut Task {
        unimplemented!()
    }

    fn add_task(&self, task: Rc<Mutex<Task>>) {
        unimplemented!()
    }

    fn start_scheduler(&self) -> ! {
        unimplemented!()
    }

    fn initialized(&self) -> bool {
        false
    }
}

static SCHEDULER: Mutex<&'static dyn Scheduler> = Mutex::new(&NullScheduler::new());

pub fn register_scheduler(scheduler: &'static dyn Scheduler) {
    *SCHEDULER.lock().unwrap() = scheduler;
}

pub fn scheduler() -> &'static dyn Scheduler {
    *SCHEDULER.lock().unwrap()
}
