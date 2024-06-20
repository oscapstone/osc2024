use alloc::vec::Vec;
use core::arch::{asm, global_asm};

use crate::println;

global_asm!(include_str!("switch.s"));

pub static mut SCHEDULER: Scheduler = Scheduler {
    run_queue: Vec::new(),
    used_thread_id: 0,
};

pub struct Thread {
    // The stack pointer of the thread
    pub sp: usize,
    pub thread_id: usize,
    // The function to run
    pub task: fn(),
    // array for callee-saved registers
    pub registers: [usize; 16],
    pub is_running: bool,
}

#[repr(C)]
struct Registers {}

impl Thread {
    pub fn new(task: fn()) -> Self {
        // allocate a memory for the thread
        Self {
            sp: 0,
            thread_id: 0,
            task,
            registers: [0; 16],
            is_running: false,
        }
    }

    pub fn run(&mut self) {
        self.is_running = true;
        set_current_tpidr_el1(self.registers.as_ptr() as u64);
        (self.task)();
    }

    pub fn get_thread_id(&self) -> usize {
        self.thread_id
    }
}

pub struct Scheduler {
    pub run_queue: Vec<Thread>,
    used_thread_id: usize,
}

fn get_current_tpidr_el1() -> u64 {
    let tpidr_el1: u64;
    unsafe {
        asm!("mrs {}, tpidr_el1", out(reg) tpidr_el1);
    }
    tpidr_el1
}

fn set_current_tpidr_el1(tpidr_el1: u64) {
    unsafe {
        asm!("msr tpidr_el1, {}", in(reg) tpidr_el1);
    }
}
#[no_mangle]
#[inline(never)]
fn schedule_boink() {
    unsafe {asm!("nop");}
}
#[no_mangle]
#[inline(never)]
pub fn schedule() {
    // get current thread id from tpidr_el1
    unsafe {
        asm!(
        "
        // save the callee-saved registers
        mrs x0, tpidr_el1
        stp x19, x20, [x0, 16 * 0]
        stp x21, x22, [x0, 16 * 1]
        stp x23, x24, [x0, 16 * 2]
        stp x25, x26, [x0, 16 * 3]
        stp x27, x28, [x0, 16 * 4]
        stp fp, lr, [x0, 16 * 5]
        mov x9, sp
        str x9, [x0, 16 * 6]
        ", out("x0") _, out("x9") _);
    }
    let cur_tpidr_el1 = get_current_tpidr_el1();
    let scheduler = unsafe { &mut SCHEDULER };
    let current_thread_pos = scheduler
        .run_queue
        .iter()
        .position(|thread| thread.registers.as_ptr() == cur_tpidr_el1 as *const usize)
        .unwrap();

    let current_thread = scheduler.run_queue.get(current_thread_pos).unwrap();
    let scheduler = unsafe { &mut SCHEDULER };

    let next_thread_pos = (current_thread_pos + 1) % scheduler.run_queue.len();
    // determine the next thread to run
    let next_thread = scheduler.run_queue.get_mut(next_thread_pos).unwrap();
    let next_thread_regs = next_thread.registers.as_ptr();
    
    println!("From thread {} to thread {}", current_thread.get_thread_id(), next_thread.get_thread_id());
    println!("current_regs: {:p}, next_regs: {:p}", cur_tpidr_el1 as *const usize, next_thread_regs);
    schedule_boink();
    set_current_tpidr_el1(next_thread_regs as u64);
    if next_thread.is_running {
        unsafe {
            asm!(
                "
                ldp x19, x20, [{0}, 16 * 0]
                ldp x21, x22, [{0}, 16 * 1]
                ldp x23, x24, [{0}, 16 * 2]
                ldp x25, x26, [{0}, 16 * 3]
                ldp x27, x28, [{0}, 16 * 4]
                ldp fp, lr, [{0}, 16 * 5]
                ldr x9, [{0}, 16 * 6]
                mov sp,  x9
                ret
                "
            ,
            in(reg) next_thread_regs, out("x9") _);
        }
    }
    else {
        next_thread.run();
    }
}
#[macro_export]
macro_rules! exit_thread {
    ($return_value:ident) => {
        let cur_thread = kernel_thread::get_scheduler().get_current_thread();
        println!("Thread {} exited", cur_thread.get_thread_id());
        cur_thread.is_running = false;
        return $return_value;
    };
    () => {
        let cur_thread = kernel_thread::get_scheduler().get_current_thread();
        println!("Thread {} exited", cur_thread.get_thread_id());
        cur_thread.is_running = false;
        return;
    };
}

pub fn get_scheduler() -> &'static mut Scheduler {
    unsafe { &mut SCHEDULER }
}

impl Scheduler {
    // if function called schedule, change to another thread.
    pub fn add_task(&mut self, task: fn()) {
        let mut thread = Thread::new(task);
        thread.thread_id = self.used_thread_id;
        self.used_thread_id += 1;
        self.run_queue.push(thread);
    }
    pub fn start(&mut self) {
        self.run_queue.first_mut().unwrap().run();
    }

    pub fn get_current_thread(&mut self) -> &mut Thread {
        let cur_tpidr_el1 = get_current_tpidr_el1();
        self.run_queue
            .iter_mut()
            .find(|thread| thread.registers.as_ptr() == cur_tpidr_el1 as *const usize)
            .unwrap()
    }

    pub fn rm_task(&mut self, task_id: usize) {
        self.run_queue.remove(task_id);
    }
}
