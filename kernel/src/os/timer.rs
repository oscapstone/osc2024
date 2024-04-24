
use alloc::collections::BinaryHeap;
use core::arch::asm;
use alloc::boxed::Box;
use super::critical_section;

static mut TimerPQ: Option<BinaryHeap<Timer>> = None;

pub trait Callback: Send + Sync {
    fn call(&self);
}

impl<F> Callback for F
where
    F: Fn() + 'static,
    F: Send + Sync,
{
    fn call(&self) {
        self();
    }
}

struct Timer {
    time: u64,
    callback: Box<dyn Callback + Send + Sync>,
}

impl Timer {
    fn trigger(&self) {
        self.callback.call();
    }
}

impl Ord for Timer {
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        self.time.cmp(&other.time).reverse() // reverse the order to make BinaryHeap a min heap
    }
}

impl PartialOrd for Timer {
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Eq for Timer {}

impl PartialEq for Timer {
    fn eq(&self, other: &Self) -> bool {
        self.time == other.time
    }
}

pub fn init() {
    unsafe {
        TimerPQ = Some(BinaryHeap::new());
        set_timer(u64::MAX);
        asm!(
            "mov {tmp}, 1",
            "msr cntp_ctl_el0, {tmp}", // Enable the timer
            // "mrs {tmp}, cntfrq_el0",
            // "msr cntp_tval_el0, {tmp}",
            "mov {tmp}, 2",
            "ldr {tmp2}, =0x40000040", // CORE0_TIMER_IRQ_CTRL
            "str {tmp}, [{tmp2}]",
            tmp = out(reg) _,
            tmp2 = out(reg) _,
        );
    }
}

pub fn get_freq() -> u64 {
    unsafe {
        let mut freq: u64;
        asm!(
            "mrs {freq}, cntfrq_el0",
            freq = out(reg) freq,
        );
        freq
    }
}

pub fn get_ticks() -> u64 {
    unsafe {
        let mut ticks: u64;
        asm!(
            "mrs {ticks}, cntpct_el0",
            ticks = out(reg) ticks,
        );
        ticks
    }
}

pub fn get_timer_ticks() -> u64 {
    unsafe {
        let mut ticks: u64;
        asm!(
            "mrs {ticks}, cntp_cval_el0",
            ticks = out(reg) ticks,
        );
        ticks
    }
}

pub fn get_time_ms() -> u64 {
    let freq = get_freq();
    let ticks = get_ticks();
    ticks / (freq / 1000)
}

pub fn add_timer_ms<F>(time: u64, callback: F) 
where
    F: Fn() + 'static,
    F: Send + Sync,
{
    let freq = get_freq();
    add_timer(get_ticks() + time * (freq / 1000), callback);
}

pub fn add_timer<F>(time: u64, callback: F )
where
    F: Fn() + 'static,
    F: Send + Sync,
{
    // println!("AT: Start");
    unsafe {
        assert_eq!(TimerPQ.is_some(), true, "TimerPQ is not initialized");

        TimerPQ.as_mut().unwrap().push(Timer { time, callback: Box::new(callback) });

        // println!("AT: Pushed");

        if let Some(&Timer { time, ref callback }) = TimerPQ.as_mut().unwrap().peek() {
            if time < get_timer_ticks() {
                set_timer(time);
                // println!("T: set");
            }
        } else {
            panic!("TimerPQ should not be empty.");
        }
        // println!("T: added");
    }
}

fn set_timer(time: u64) {
    unsafe {
        asm!(
            "msr cntp_cval_el0, {time}",
            time = in(reg) time,
        );
    }
}

pub unsafe fn irq_handler() {
    // println!("IRQ handler");
    loop {
        if let Some(&Timer { time, ref callback }) = TimerPQ.as_mut().unwrap().peek() {
            if time < get_ticks() {
                // for c in b"T: callback start\r\n" {
                //     crate::cpu::uart::send(*c);
                // }

                callback.call();

                // for c in b"T: callback end\r\n" {
                //     crate::cpu::uart::send(*c);
                // }
                TimerPQ.as_mut().unwrap().pop();
            } else {
                set_timer(time);
                break;
            }
        } else {

            // No more timers
            set_timer(u64::MAX);
            break;
        }
    }
}