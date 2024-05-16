use aarch64_cpu::registers::{Writeable, SPSR_EL1, TPIDR_EL1};
use alloc::{
    boxed::Box,
    collections::{LinkedList, VecDeque},
    rc::Rc,
};
use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt, run_user_code};
use library::sync::mutex::Mutex;

use crate::{
    driver::timer,
    memory::{phys_binary_load_addr, phys_dram_start_addr},
};

use super::{
    current, switch_to,
    task::{StackInfo, Task, TaskState},
    Scheduler,
};

pub struct RoundRobinScheduler {
    run_queue: Mutex<VecDeque<Rc<Mutex<Task>>>>,
    wait_queue: Mutex<LinkedList<Rc<Mutex<Task>>>>,
    initialized: Mutex<bool>,
}

impl RoundRobinScheduler {
    pub const fn new() -> Self {
        Self {
            run_queue: Mutex::new(VecDeque::new()),
            wait_queue: Mutex::new(LinkedList::new()),
            initialized: Mutex::new(false),
        }
    }
}

impl RoundRobinScheduler {
    fn idle(&self) -> ! {
        loop {
            let mut i = 0;
            while i < {
                unsafe { disable_kernel_space_interrupt() };
                self.run_queue.lock().unwrap().len()
            } {
                {
                    let task_ref = self.run_queue.lock().unwrap()[i].clone();
                    let task = task_ref.lock().unwrap();
                    if task.state() == TaskState::Dead {
                        self.run_queue.lock().unwrap().remove(i);
                    }
                }
                unsafe { enable_kernel_space_interrupt() };
                i += 1;
            }
            self._schedule();
        }
    }

    /// Schedule the next task to run
    /// Caller should ensure the interrupt is enabled after context switch
    fn _schedule(&self) -> *mut Task {
        loop {
            // protect the scheduler from being interrupted
            unsafe { disable_kernel_space_interrupt() };
            if let Some(mut next_task) = self.run_queue.lock().unwrap().pop_front() {
                let next_task_state = next_task.lock().unwrap().state();
                let next = &mut *next_task.lock().unwrap() as *mut Task;
                self.run_queue.lock().unwrap().push_back(next_task);
                if next_task_state == TaskState::Running {
                    return unsafe { switch_to(current(), next) };
                }
            } else {
                unsafe { enable_kernel_space_interrupt() };
                panic!("No task to run!");
            }
        }
    }
}

impl Scheduler for RoundRobinScheduler {
    fn schedule(&self) -> *mut Task {
        unsafe { disable_kernel_space_interrupt() };
        let next = self._schedule();
        unsafe { enable_kernel_space_interrupt() };
        next
    }

    fn add_task(&self, task: Rc<Mutex<Task>>) {
        unsafe { disable_kernel_space_interrupt() };
        task.lock()
            .unwrap()
            .pid()
            .lock()
            .unwrap()
            .set_pid_task(&task);
        self.run_queue.lock().unwrap().push_back(task);
        unsafe { enable_kernel_space_interrupt() };
    }

    fn start_scheduler(&self) -> ! {
        let idle_task = Rc::new(Mutex::new(Task::new(StackInfo::new(
            phys_dram_start_addr() as *mut u8,
            phys_binary_load_addr() as *mut u8,
        ))));
        idle_task
            .lock()
            .unwrap()
            .pid()
            .lock()
            .unwrap()
            .set_pid_task(&idle_task);
        let idle_task_ptr = &*idle_task.lock().unwrap() as *const Task;
        {
            let mut run_queue = self.run_queue.lock().unwrap();
            // set the idle task as the first task, it will sync thread context automatically
            run_queue.push_front(idle_task);
            // set the idle task as current task
            aarch64_cpu::registers::TPIDR_EL1.set(idle_task_ptr as u64);
        }
        timer().set_timeout_raw(
            timer().tick_per_second() >> 5,
            Box::new(scheduler_timer_handler),
        );
        self._schedule();
        *self.initialized.lock().unwrap() = true;
        self.idle();
    }

    fn execute_task(&self, mut task: Task) {
        const USER_STACK_SIZE: usize = 4096;
        let stack_end = Box::into_raw(Box::new([0_u8; USER_STACK_SIZE])) as u64;
        let code_start = task.thread.context.pc;
        {
            task.thread.spsr_el1 = (SPSR_EL1::D::Masked
                + SPSR_EL1::I::Unmasked
                + SPSR_EL1::A::Masked
                + SPSR_EL1::F::Masked
                + SPSR_EL1::M::EL0t)
                .into();
            task.thread.elr_el1 = code_start;
            task.thread.sp_el0 = stack_end;
        }
        let t = Rc::new(Mutex::new(task));
        let task_ptr = &*t.lock().unwrap() as *const Task;
        unsafe { disable_kernel_space_interrupt() };
        TPIDR_EL1.set(task_ptr as u64);
        self.run_queue.lock().unwrap().push_front(t);
        unsafe { enable_kernel_space_interrupt() };
        unsafe { run_user_code(stack_end, code_start) };
    }

    fn initialized(&self) -> bool {
        *self.initialized.lock().unwrap()
    }
}

fn scheduler_timer_handler() -> Result<(), &'static str> {
    let interval = timer().tick_per_second() >> 5;
    timer().set_timeout_raw(interval, Box::new(scheduler_timer_handler));
    ROUND_ROBIN_SCHEDULER.schedule();
    Ok(())
}

pub static ROUND_ROBIN_SCHEDULER: RoundRobinScheduler = RoundRobinScheduler::new();
