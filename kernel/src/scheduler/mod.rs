use crate::{exception, thread::Thread};
use alloc::boxed::Box;
use alloc::collections::VecDeque;
use alloc::vec::Vec;
use core::arch::asm;
use core::time::Duration;
use stdio::{debug, println};

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

    pub fn add(&mut self, thread: Box<Thread>) {
        let pos = self.threads.iter().position(|t| t.is_none());
        match pos {
            Some(index) => {
                self.threads[index] = Some(thread);
                self.ready_queue.push_back(index);
            }
            None => {
                self.threads.push(Some(thread));
                self.ready_queue.push_back(self.threads.len() - 1);
            }
        }
    }
    pub fn schedule(&mut self) {
        self.sched_timer();

        if self.current.is_none() {}

        println!("{} threads in ready queue", self.ready_queue.len());
        if self.ready_queue.is_empty() {
            println!("No thread to schedule");
            return;
        }
        if self.current.is_none() {
            let next = self.ready_queue.pop_front().unwrap();
            self.current = Some(next);
            println!("Switching to {}", next);
            unsafe {
                exception::enable_interrupt();
                let next: *const Thread = self.threads[next].as_ref().unwrap().as_ref();
                asm!("mov x1, {}", in(reg) next, out("x1") _);
                asm!("bl context_switch");
                exception::disable_interrupt();
            }
            return;
        }
        let current = self.current.unwrap();
        let next = self.ready_queue.pop_front().unwrap();
        if let Some(_) = self.threads[next].as_ref() {
            self.ready_queue.push_back(current);
            self.current = Some(next);
            println!("Switching from {} to {}", current, next);
            unsafe {
                exception::enable_interrupt();
                let prev: *const Thread = self.threads[current].as_ref().unwrap().as_ref();
                let next: *const Thread = self.threads[next].as_ref().unwrap().as_ref();
                asm!("mov x0, {}", in(reg) prev, out("x0") _);
                asm!("mov x1, {}", in(reg) next, out("x1") _);
                asm!("bl context_switch");
                exception::disable_interrupt();
            }
        }
    }

    pub fn create_thread(&mut self, entry: extern "C" fn(*mut u8), args: *mut u8) {
        let thread = Box::new(Thread::new(self.threads.len() as u32, 0x2000, entry, args));
        println!("Created thread {}", thread.id);
        self.add(thread);
        println!("Number of threads: {}", self.threads.len());
    }

    pub fn sched_timer(&mut self) {
        let tm = crate::timer::manager::get();
        tm.add_timer(
            Duration::from_secs_f64(1 as f64 / (1 << 2) as f64),
            Box::new(|| {
                get().schedule();
            }),
        );
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
    (thread.entry)(thread.args);
    println!("Thread {} exited", thread.id);
    panic!("Thread exited");
    loop {
        unsafe {
            asm!("wfe");
        }
    }
}

static mut SCHEDULER: Option<Scheduler> = None;

pub fn get() -> &'static mut Scheduler {
    unsafe { SCHEDULER.as_mut().unwrap() }
}

pub fn init() {
    unsafe {
        SCHEDULER = Some(Scheduler::new());
    }
    // let tm = crate::timer::manager::get();
    // tm.add_timer(
    //     Duration::from_millis(1000),
    //     Box::new(|| {
    //         get().sched_timer();
    //         println!("Switching to {}", get().current.unwrap());
    //         unsafe {
    //             let current: *const Thread = get().threads[get().current.unwrap()]
    //                 .as_ref()
    //                 .unwrap()
    //                 .as_ref();
    //             exception::enable_interrupt();
    //             asm!("mov x1, {}", in(reg) current, out("x1") _);
    //             asm!("bl load_cpu_status");
    //             exception::disable_interrupt();
    //             return;
    //         }
    //     }),
    // );
    // assert!(get().ready_queue.is_empty());
    // assert!(get().current.is_none());
    // get().create_thread(idle, 0 as *mut u8);
    // get().current = Some(0);
}

extern "C" fn idle(_: *mut u8) {
    println!("Idle thread");
    loop {
        unsafe {
            asm!("wfe");
        }
    }
}
