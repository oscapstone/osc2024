use crate::exception::trap_frame::TRAP_FRAME;
use crate::thread::state;
use crate::thread::Thread;
use alloc::boxed::Box;
use alloc::collections::VecDeque;
use alloc::vec::Vec;
use core::arch::asm;
use core::time::Duration;
use stdio::println;
pub struct Scheduler {
    pub current: Option<usize>,
    pub threads: Vec<Option<Box<Thread>>>,
    pub ready_queue: VecDeque<usize>,
    pub zombie_queue: VecDeque<usize>,
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

    fn add_thread(&mut self, mut thread: Box<Thread>) -> usize {
        let pos = self.threads.iter().position(|t| t.is_none());
        match pos {
            Some(index) => {
                thread.id = index;
                self.threads[index] = Some(thread);
                index
            }
            None => {
                thread.id = self.threads.len();
                self.threads.push(Some(thread));
                self.threads.len() - 1
            }
        }
    }

    fn save_current(&mut self) -> usize {
        let current = self.current.unwrap();
        unsafe {
            self.threads[current].as_mut().unwrap().cpu_state = TRAP_FRAME.as_ref().unwrap().state;
        }
        current
    }

    fn restore_next(&mut self) -> usize {
        if let Some(next) = self.ready_queue.pop_front() {
            unsafe {
                TRAP_FRAME.as_mut().unwrap().state = self.threads[next].as_ref().unwrap().cpu_state;
            }
            next
        } else {
            panic!("No thread to restore");
        }
    }

    pub fn schedule(&mut self) {
        // println!("{} threads in ready queue", self.ready_queue.len());
        assert!(self.current.is_some());
        if self.ready_queue.is_empty() {
            // println!("No thread to schedule");
            return;
        }
        let current = self.save_current();
        let next = self.restore_next();
        self.current = Some(next);
        self.ready_queue.push_back(current);
        println!("Switching from {} to {}", current, next);
    }

    pub fn create_thread(&mut self, entry: extern "C" fn()) {
        let thread = Box::new(Thread::new(0x2000, entry));
        println!("Created thread {}", thread.id);
        let tid = self.add_thread(thread);
        self.ready_queue.push_back(tid);
        println!("Number of threads: {}", self.threads.len());
    }

    pub fn sched_timer(&mut self) {
        let tm = crate::timer::manager::get();
        tm.add_timer(
            Duration::from_secs_f64(1 as f64 / (1 << 5) as f64),
            Box::new(|| {
                get().schedule();
                get().sched_timer();
            }),
        );
    }

    pub fn run_threads(&mut self) {
        self.sched_timer();
        assert!(self.current.is_none());
        assert!(!self.ready_queue.is_empty());
        let next = self.ready_queue.pop_front().unwrap();
        self.current = Some(next);
        println!("Switching to {}", next);
        let thread = self.threads[next].as_ref().unwrap();
        let entry = thread.entry;
        let sp = thread.stack as usize + thread.stack_size;
        unsafe {
            asm!(
                "msr spsr_el1, xzr",
                "msr elr_el1, {0}",
                "msr sp_el0, {1}",
                "eret",
                in(reg) entry,
                in(reg) sp,
            );
        }
    }

    pub fn fork(&mut self) -> u64 {
        self.save_current();
        let current = self.current.unwrap();
        let mut new_thread = self.threads[current].as_ref().unwrap().clone();
        new_thread.cpu_state.x[0] = 0;
        self.add_thread(new_thread) as u64
    }

    pub fn exit(&mut self, status: u64) {
        let current = self.current.unwrap();
        self.threads[current].as_mut().unwrap().state = state::State::Zombie;
        self.zombie_queue.push_back(current);
        println!("Thread {} exited with status {}", current, status);
        self.current = None;
        self.sched_timer();
        if self.ready_queue.is_empty() {
            panic!("All threads exited");
        }
        let next = self.restore_next();
        self.current = Some(next);
    }
}

pub extern "C" fn get_current() -> *mut Thread {
    let current: *mut Thread;
    unsafe {
        asm!("mrs {0}, tpidr_el1", out(reg) current);
    }
    current
}

#[no_mangle]
pub extern "C" fn thread_wrapper() {
    let thread = get_current();
    let thread = unsafe { &mut *thread };
    (thread.entry)();
    println!("Thread {} exited", thread.id);
    panic!("Thread exited");
}

static mut SCHEDULER: Option<Scheduler> = None;

pub fn get() -> &'static mut Scheduler {
    unsafe { SCHEDULER.as_mut().unwrap() }
}

pub fn init() {
    unsafe {
        SCHEDULER = Some(Scheduler::new());
    }
}
