use crate::println;
use alloc::collections::BinaryHeap;
use alloc::string::String;
use core::{panic, ptr::write_volatile};
// globle variable

struct TimerEntry {
    pub target_time: u64,
    pub callback: fn(String),
    pub message: String,
}

// Ord
impl core::cmp::Ord for TimerEntry {
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        self.target_time.cmp(&other.target_time).reverse()
    }
}

// PartialOrd
impl core::cmp::PartialOrd for TimerEntry {
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

// PartialEq
impl core::cmp::PartialEq for TimerEntry {
    fn eq(&self, other: &Self) -> bool {
        self.target_time == other.target_time
    }
}

// Eq
impl core::cmp::Eq for TimerEntry {}

static mut TIMER_ENTRY_QUEUE: Option<BinaryHeap<TimerEntry>> = None;

pub fn add_timer(callback: fn(String), time_sec: u64, message: String) {
    let current_time = get_current_time();
    let time_duration = sec_to_tick(time_sec);
    let target_time = time_duration + current_time;
    let mut entry: TimerEntry = TimerEntry {
        target_time: target_time, // the time to trigger the timer
        callback: callback,
        message: message,
    };
    unsafe {
        set_timmer_interrupt(time_duration);
        if TIMER_ENTRY_QUEUE.is_none() {
            TIMER_ENTRY_QUEUE = Some(BinaryHeap::new());
        }
        if TIMER_ENTRY_QUEUE.as_mut().unwrap().is_empty() {
        }
        TIMER_ENTRY_QUEUE.as_mut().unwrap().push(entry);
    }
}

#[no_mangle]
#[inline(never)]
fn timer_boink() {
    unsafe { core::arch::asm!("nop") };
}

pub fn timer_handler() {
    timer_boink();
    driver::uart::uart_write_str("Timer interrupt!\n");
    if let Some(queue) = unsafe {TIMER_ENTRY_QUEUE.as_mut()} {
        if let Some(entry) = queue.pop() {
            // execute the callback function
            (entry.callback)(entry.message.clone());
            if let Some(next_entry) = queue.peek() {
                set_timmer_interrupt(next_entry.target_time - get_current_time());
            } else {
                // queue is empty
                disable_timmer_interrupt();
            }
        } else {
            panic!("No timer entry!");
        }
        
    }
}

pub fn get_timer_freq() -> u64 {
    let mut cntfrq_el0: u64;
    unsafe {
        core::arch::asm!("mrs {0}, cntfrq_el0", out(reg) cntfrq_el0);
    }
    cntfrq_el0
}

pub fn get_current_time() -> u64 {
    let mut cntpct_el0: u64;
    unsafe {
        core::arch::asm!("mrs {0}, cntpct_el0", out(reg) cntpct_el0);
    }
    cntpct_el0
}

pub fn sec_to_tick(sec: u64) -> u64 {
    sec * get_timer_freq()
}

fn timer_callback(message: String) {
    let current_time = get_current_time() / get_timer_freq();
    println!("You have a timer after boot {}s, message: {}", current_time, message);
}

// pub fn test_binary_heap() {
//     add_timer(timer_callback, 5, String::from("Hello, world! 5s"));
//     add_timer(timer_callback, 4, String::from("Hello, world! 4s"));
//     add_timer(timer_callback, 10, String::from("Hello, world! 10s"));
//     add_timer(timer_callback, 3, String::from("Hello, world! 3s"));
//     add_timer(timer_callback, 2, String::from("Hello, world! 2s"));
//     add_timer(timer_callback, 9, String::from("Hello, world! 9s"));
//     loop {
//         if let Some(entry) = unsafe { TIMER_ENTRY_QUEUE.as_mut().unwrap().pop() } {
//             println!("target_time: {}", entry.target_time);
//         } else {
//             break;
//         }
//     }
// }

fn set_timmer_interrupt(time: u64) {
    unsafe {
        let addr: *mut u32 = 0x40000040 as *mut u32;
        let basic_irq_enable: *mut u32 = 0x3F00B218 as *mut u32;
        core::arch::asm!("msr cntp_tval_el0, {0}", in(reg) time);
        println!(
            "Debug: Set timer interrupt after {} tic, freq: {}",
            time,
            get_timer_freq()
        );
        // enable timer interrupt
        // write_volatile(basic_irq_enable, 0b1);
        // enable the timer
        write_volatile(addr, 0x2);
    }
}
fn disable_timmer_interrupt() {
    unsafe {
        let addr: *mut u32 = 0x40000040 as *mut u32;
        let basic_irq_enable: *mut u32 = 0x3F00B218 as *mut u32;
        // enable timer interrupt
        // write_volatile(basic_irq_enable, 0b0);
        // disable the timer
        write_volatile(addr, 0x0);
    }
}
