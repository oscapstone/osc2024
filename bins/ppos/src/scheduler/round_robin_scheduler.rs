use core::slice;

use aarch64_cpu::registers::Writeable;
use alloc::{
    boxed::Box,
    collections::{LinkedList, VecDeque},
    rc::Rc,
};
use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};
use library::{
    console::{console, ConsoleMode},
    println,
    sync::mutex::Mutex,
};

use crate::{
    driver::timer,
    memory::{virtual_kernel_space_addr, virtual_kernel_start_addr, AllocatedMemory},
    pid::pid_manager,
};

use super::{
    current, switch_to,
    task::{Task, TaskState},
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
                        pid_manager().remove_pid(task.pid().lock().unwrap().number());
                        console().change_mode(ConsoleMode::Sync);
                        println!("Task {} is recycled", task.pid().lock().unwrap().number());
                        console().change_mode(ConsoleMode::Async);
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
            if let Some(next_task) = self.run_queue.lock().unwrap().pop_front() {
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
        let idle_task = Rc::new(Mutex::new(Task::new(
            // the idle task will never be recycled so it's safe
            AllocatedMemory::new(unsafe {
                Box::from_raw(slice::from_raw_parts_mut(
                    virtual_kernel_space_addr() as *mut u8,
                    virtual_kernel_start_addr() - virtual_kernel_space_addr(),
                ) as *mut [u8])
            }),
            None,
        )));
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
