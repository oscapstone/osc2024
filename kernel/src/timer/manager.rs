use super::timer::Timer;
use alloc::boxed::Box;
use alloc::collections::BinaryHeap;
use core::{arch::asm, time::Duration};
use stdio::*;
pub struct TimerManager {
    pq: BinaryHeap<Timer>,
}

pub static mut TIMER_MANAGER: Option<TimerManager> = None;

pub fn init_timer_manager() {
    unsafe {
        TIMER_MANAGER = Some(TimerManager::new());
    }
}

pub fn get_timer_manager() -> &'static mut TimerManager {
    unsafe { TIMER_MANAGER.as_mut().unwrap() }
}

unsafe fn enable_timmer_irq() {
    // debug!("Enable timer interrupt");
    asm!(
        "mov {0}, 1",
        "msr cntp_ctl_el0, {0}",
        "mov {0}, 2",
        "ldr {1}, =0x40000040",
        "str {0}, [{1}]",
        out(reg) _,
        out(reg) _,
    );
}

unsafe fn disable_timmer_irq() {
    // debug!("Disable timer interrupt");
    asm!(
        "mov {0}, 0",
        "msr cntp_ctl_el0, {0}",
        "ldr {1}, =0x40000040",
        "str {0}, [{1}]",
        out(reg) _,
        out(reg) _,
    );
}

impl TimerManager {
    pub fn new() -> Self {
        let ret = TimerManager {
            pq: BinaryHeap::new(),
        };
        return ret;
    }

    #[inline(never)]
    pub fn add_timer(&mut self, duration: Duration, callback: Box<dyn Fn() + Send + Sync>) {
        let expiry = self.compute_delay(duration);
        let timer = Timer::new(expiry, callback);
        self.pq.push(timer);
        self.set_timer();
        if self.pq.len() == 1 {
            unsafe {
                enable_timmer_irq();
            }
        }
    }

    pub fn handle_inturrupt(&mut self) {
        if let Some(timer) = self.pq.pop() {
            assert!(
                timer.expiry <= self.get_current(),
                "Timer expired too early"
            );
            timer.trigger();
            if self.pq.len() > 0 {
                self.set_timer();
            } else {
                unsafe {
                    disable_timmer_irq();
                }
            }
        } else {
            unsafe {
                disable_timmer_irq();
            }
        }
    }

    fn set_timer(&self) {
        if let Some(timer) = self.pq.peek() {
            unsafe {
                asm!(
                    "msr cntp_cval_el0, {0}",
                    in(reg) timer.expiry,
                );
            }
        } else {
            println!("No timer to set")
        }
    }

    fn get_current(&self) -> u64 {
        let mut now: u64;
        unsafe {
            asm!(
                "mrs {0}, cntpct_el0",
                out(reg) now,
            )
        }
        now
    }

    fn get_frequency(&self) -> u64 {
        let mut freq: u64;
        unsafe {
            asm!(
                "mrs {0}, cntfrq_el0",
                out(reg) freq,
            )
        }
        freq
    }

    fn compute_delay(&self, duration: Duration) -> u64 {
        let now = self.get_current();
        let freq = self.get_frequency();
        let delay = duration.as_secs() as u64;
        now + (delay * freq)
    }
}
