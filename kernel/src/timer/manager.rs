use super::timer::Timer;
use spin::Mutex;

use alloc::collections::BinaryHeap;

pub struct TimerManager {
    pq: BinaryHeap<Timer>,
}

static TIMER_MANAGER: Mutex<Option<TimerManager>> = Mutex::new(None);

pub fn initialize_timers() {
    *TIMER_MANAGER.lock() = Some(TimerManager::new());
}

pub fn get_timer_manager() -> &'static Mutex<Option<TimerManager>> {
    &TIMER_MANAGER
}

impl TimerManager {
    pub fn new() -> TimerManager {
        TimerManager {
            pq: BinaryHeap::new(),
        }
    }

    pub fn add_timer(&mut self, timer: Timer) {
        self.pq.push(timer);
    }

    pub fn pop_timer(&mut self) -> Option<Timer> {
        self.pq.pop()
    }
}
