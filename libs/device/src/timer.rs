use aarch64_cpu::registers::*;
use alloc::{boxed::Box, collections::LinkedList};
use core::time::Duration;
use library::sync::mutex::Mutex;
use tock_registers::{
    interfaces::ReadWriteable as _, register_bitfields, register_structs, registers::ReadWrite,
};

use crate::{
    common::MMIODerefWrapper,
    device_driver::DeviceDriver,
    interrupt_controller::InterruptNumber,
    interrupt_manager::{self, InterruptHandler, InterruptHandlerDescriptor, InterruptPrehook},
};

register_bitfields! [
    u32,
    TIMER_INTERRUPT_CONTROLL [
        CNTVIRQ_FIQ_CONTROLL OFFSET(7) NUMBITS(1) [
            FIQ_DISABLE = 0,
            FIQ_ENABLE = 1
        ],
        CNTHPIRQ_FIQ_CONTROLL OFFSET(6) NUMBITS(1) [
            FIQ_DISABLE = 0,
            FIQ_ENABLE = 1
        ],
        CNTPNSIRQ_FIQ_CONTROLL OFFSET(5) NUMBITS(1) [
            FIQ_DISABLE = 0,
            FIQ_ENABLE = 1
        ],
        CNTPSIRQ_FIQ_CONTROLL OFFSET(4) NUMBITS(1) [
            FIQ_DISABLE = 0,
            FIQ_ENABLE = 1
        ],
        CNTVIRQ_IRQ_CONTROLL OFFSET(3) NUMBITS(1) [
            IRQ_DISABLE = 0,
            IRQ_ENABLE = 1
        ],
        CNTHPIRQ_IRQ_CONTROLL OFFSET(2) NUMBITS(1) [
            IRQ_DISABLE = 0,
            IRQ_ENABLE = 1
        ],
        CNTPNSIRQ_IRQ_CONTROLL OFFSET(1) NUMBITS(1) [
            IRQ_DISABLE = 0,
            IRQ_ENABLE = 1
        ],
        CNTPSIRQ_IRQ_CONTROLL OFFSET(0) NUMBITS(1) [
            IRQ_DISABLE = 0,
            IRQ_ENABLE = 1
        ]
    ],
];

register_structs! {
    CoreTimerInterruptControllRegisters {
        (0x00 => core0_timer_interrupt_controll: ReadWrite<u32, TIMER_INTERRUPT_CONTROLL::Register>),
        (0x04 => core1_timer_interrupt_controll: ReadWrite<u32, TIMER_INTERRUPT_CONTROLL::Register>),
        (0x08 => core2_timer_interrupt_controll: ReadWrite<u32, TIMER_INTERRUPT_CONTROLL::Register>),
        (0x0c => core3_timer_interrupt_controll: ReadWrite<u32, TIMER_INTERRUPT_CONTROLL::Register>),
        (0x10 => @END),
    }
}

pub struct Timer {
    core_timer_interrupt_controll_registers:
        Mutex<MMIODerefWrapper<CoreTimerInterruptControllRegisters>>,
    timeout_handler_list: Mutex<LinkedList<TimeoutDescriptor>>,
}

pub struct TimeoutDescriptor {
    time: u64,
    handler: Box<TimeoutHandler>,
}

impl TimeoutDescriptor {
    pub const fn new(time: u64, handler: Box<TimeoutHandler>) -> Self {
        Self { time, handler }
    }
}

pub type TimeoutHandler = dyn Fn() -> Result<(), &'static str>;

impl Timer {
    pub const unsafe fn new(
        core_timer_interrupt_controll_registers_mmio_start_addr: usize,
    ) -> Self {
        Self {
            core_timer_interrupt_controll_registers: Mutex::new(MMIODerefWrapper::new(
                core_timer_interrupt_controll_registers_mmio_start_addr,
            )),
            timeout_handler_list: Mutex::new(LinkedList::new()),
        }
    }

    #[inline(always)]
    fn enable_timer_interrupt(&self) {
        CNTP_CTL_EL0.write(CNTP_CTL_EL0::ENABLE::SET + CNTP_CTL_EL0::IMASK::CLEAR);
        self.core_timer_interrupt_controll_registers
            .lock()
            .unwrap()
            .core0_timer_interrupt_controll
            .modify(
                TIMER_INTERRUPT_CONTROLL::CNTPNSIRQ_FIQ_CONTROLL::FIQ_DISABLE
                    + TIMER_INTERRUPT_CONTROLL::CNTPNSIRQ_IRQ_CONTROLL::IRQ_ENABLE,
            );
    }

    #[inline(always)]
    fn disable_timer_interrupt(&self) {
        self.core_timer_interrupt_controll_registers
            .lock()
            .unwrap()
            .core0_timer_interrupt_controll
            .modify(TIMER_INTERRUPT_CONTROLL::CNTPNSIRQ_IRQ_CONTROLL::IRQ_DISABLE);
    }

    #[inline(always)]
    fn current_time(&self) -> u64 {
        CNTPCT_EL0.get()
    }

    #[inline(always)]
    fn tick_per_second(&self) -> u64 {
        CNTFRQ_EL0.get()
    }

    #[inline(always)]
    fn set_timeout_after(&self, duration: Duration) {
        CNTP_TVAL_EL0.set(self.tick_per_second() * duration.as_secs());
    }

    #[inline(always)]
    fn set_timeout_at(&self, time: u64) {
        CNTP_CVAL_EL0.set(time);
    }

    pub fn set_timeout(&self, duration: Duration, handler: Box<TimeoutHandler>) {
        let time_to_run_handler = self.current_time() + self.tick_per_second() * duration.as_secs();
        let timeout_descriptor = TimeoutDescriptor::new(time_to_run_handler, handler);
        self.disable_timer_interrupt();
        let mut timeout_handler_list = self.timeout_handler_list.lock().unwrap();
        if let Some(descriptor) = timeout_handler_list.front() {
            if descriptor.time > time_to_run_handler {
                timeout_handler_list.push_front(timeout_descriptor);
                self.set_timeout_at(time_to_run_handler);
            } else {
                timeout_handler_list.push_back(timeout_descriptor);
            }
        } else {
            timeout_handler_list.push_back(timeout_descriptor);
            self.set_timeout_at(time_to_run_handler);
        }
        self.enable_timer_interrupt();
    }
}

impl InterruptPrehook for Timer {
    fn prehook(&self) -> Result<(), &'static str> {
        self.disable_timer_interrupt();
        Ok(())
    }
}

impl InterruptHandler for Timer {
    fn handle(&self) -> Result<(), &'static str> {
        {
            let timeout_handler_list = self.timeout_handler_list.lock().unwrap();
            if timeout_handler_list.is_empty() {
                self.disable_timer_interrupt();
                return Ok(());
            }
            let current_time = self.current_time();
            {
                let timeout_descriptor = timeout_handler_list.front().unwrap();
                if current_time < timeout_descriptor.time {
                    self.set_timeout_at(timeout_descriptor.time);
                    return Ok(());
                }
            }
        }
        // critical section
        // timer interrupt may be nested
        self.disable_timer_interrupt();
        let mut timeout_handler_list = self.timeout_handler_list.lock().unwrap();
        let timeout_descriptor = timeout_handler_list.pop_front().unwrap();
        if let Some(next_timeout_descriptor) = timeout_handler_list.front() {
            self.set_timeout_at(next_timeout_descriptor.time);
        }
        self.enable_timer_interrupt();
        (timeout_descriptor.handler)()
    }
}

impl DeviceDriver for Timer {
    type InterruptNumberType = InterruptNumber;

    fn register_and_enable_interrupt_handler(
        &'static self,
        interrupt_number: &Self::InterruptNumberType,
    ) -> Result<(), &'static str> {
        let descriptor =
            InterruptHandlerDescriptor::new(*interrupt_number, "timer", Some(self), self, 1);
        interrupt_manager::interrupt_manager().register_handler(descriptor)?;
        self.enable_timer_interrupt();
        Ok(())
    }
}
