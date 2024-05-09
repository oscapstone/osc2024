use crate::exception;
use crate::thread::Thread;
use crate::timer::manager::get_timer_manager;
use alloc::boxed::Box;
use alloc::collections::VecDeque;
use alloc::vec::Vec;
use core::time::Duration;
use core::{arch::asm, panic};
use stdio::{debug, println};
pub struct Scheduler {
    pub current: Option<Box<Thread>>,
    pub threads: Vec<Box<Thread>>,
    pub ready_queue: VecDeque<Box<Thread>>,
    pub zombie_queue: VecDeque<Box<Thread>>,
}

impl Scheduler {
    fn new() -> Self {
        Scheduler {
            current: None,
            threads: Vec::new(),
            ready_queue: VecDeque::new(),
            zombie_queue: VecDeque::new(),
        }
    }

    pub fn add(&mut self, thread: Box<Thread>) {
        self.threads.push(thread);
        self.ready_queue
            .push_back(self.threads.last().unwrap().clone());
    }

    pub fn schedule(&mut self) {
        let tm = get_timer_manager();
        tm.add_timer(
            Duration::from_millis(1000),
            Box::new(|| {
                let scheduler = get_scheduler();
                scheduler.schedule();
            }),
        );
        unsafe {
            exception::enable_inturrupt();
        }
        tm.print();
        if let Some(mut current) = self.current.take() {
            match current.state {
                super::thread::state::State::Running => {
                    current.state = super::thread::state::State::Ready;
                    self.ready_queue.push_back(current.clone());
                    if let Some(mut next) = self.ready_queue.pop_front() {
                        next.state = super::thread::state::State::Running;
                        self.current = Some(next.clone());
                        unsafe {
                            asm!("mov x0, {0}", in(reg) (&*current as *const crate::thread::Thread as usize));
                            asm!("mov x1, {0}", in(reg) (&*next as *const crate::thread::Thread as usize));
                            asm!("b save_cpu_status");
                        }
                        debug!("Thread switched");
                    } else {
                        panic!("No thread to run");
                    }
                }
                super::thread::state::State::Zombie => {
                    self.zombie_queue.push_back(current.clone());
                    panic!("Zombie thread found");
                }
                _ => {
                    panic!("Invalid state");
                }
            }
        } else {
            println!("No current thread found, starting the first thread.");
            if let Some(mut next) = self.ready_queue.pop_front() {
                next.state = super::thread::state::State::Running;
                self.current = Some(next.clone());
                unsafe {
                    asm!(
                        "mov x1, {0}",
                        "b load_cpu_status",
                        in(reg) (&*next as *const crate::thread::Thread as usize),
                    );
                }
                panic!("Thread started, (should not be printed)");
            }
        }
    }

    pub fn create_thread(&mut self, entry: extern "C" fn(*mut u8), args: *mut u8) {
        let thread = Box::new(Thread::new(self.threads.len() as u32, 0x2000, entry, args));
        self.add(thread);
    }
}

pub extern "C" fn get_current() -> *mut Thread {
    let current: *mut Thread;
    unsafe {
        asm!("mrs {0}, tpidr_el1", out(reg) current);
    }
    current
}

pub extern "C" fn thread_wrapper() {
    let thread = get_current();
    let thread = unsafe { &mut *thread };
    (thread.entry)(thread.args);
    println!("Thread {} exited", thread.id);
    loop {
        unsafe {
            asm!("wfe");
        }
    }
}

static mut SCHEDULER: Option<Scheduler> = None;

pub fn get_scheduler() -> &'static mut Scheduler {
    unsafe { SCHEDULER.as_mut().unwrap() }
}

pub fn scheduler_init() {
    unsafe {
        SCHEDULER = Some(Scheduler::new());
    }
}
