use alloc::collections::LinkedList;
use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};
use library::sync::mutex::Mutex;

use crate::interrupt_manager::InterruptHandler;
use crate::local_interrupt_controller::LocalInterruptController;
use crate::{
    common::BoundedUsize, interrupt_manager::InterruptHandlerDescriptor,
    peripheral_ic::PeripheralIC,
};
use crate::{device_driver::DeviceDriver, interrupt_manager::InterruptManager};

pub type LocalInterrupt = BoundedUsize<0, { InterruptController::MAX_LOCAL_INTERRUPT_NUMBER }>;
pub type PeripheralInterrupt = BoundedUsize<
    { InterruptController::MAX_LOCAL_INTERRUPT_NUMBER + 1 },
    { InterruptController::MAX_PERIPHERAL_INTERRUPT_NUMBER },
>;

#[derive(Copy, Clone)]
#[repr(usize)]
pub enum LocalInterruptType {
    Timer1 = 1,
    Timer3 = 3,
}

#[derive(Copy, Clone)]
#[repr(usize)]
pub enum PeripherialInterruptType {
    Aux = 29,
}

#[derive(Copy, Clone)]
pub enum InterruptNumber {
    Local(LocalInterrupt),
    Peripheral(PeripheralInterrupt),
}

impl core::fmt::Display for InterruptNumber {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            InterruptNumber::Local(n) => write!(f, "Local interrupt {}", n),
            InterruptNumber::Peripheral(n) => write!(f, "Peripheral interrupt {}", n),
        }
    }
}

pub struct PendingInterrupts {
    bitmask: u64,
}

impl PendingInterrupts {
    pub fn new(bitmask: u64) -> Self {
        Self { bitmask }
    }
}

impl Iterator for PendingInterrupts {
    type Item = usize;

    fn next(&mut self) -> Option<Self::Item> {
        if self.bitmask == 0 {
            return None;
        }

        let next = self.bitmask.trailing_zeros() as usize;
        self.bitmask &= self.bitmask.wrapping_sub(1);
        Some(next)
    }
}

pub struct PendingInterruptHandlerDescriptor {
    handler: &'static (dyn InterruptHandler + Send + Sync),
    priority: usize,
}

impl PendingInterruptHandlerDescriptor {
    pub const fn new(
        handler: &'static (dyn InterruptHandler + Send + Sync),
        priority: usize,
    ) -> Self {
        Self { handler, priority }
    }
}

pub struct PendingInterruptQueue {
    queue: Mutex<LinkedList<PendingInterruptHandlerDescriptor>>,
    current_interrupt_priority: Mutex<usize>,
}

impl PendingInterruptQueue {
    pub const fn new() -> Self {
        Self {
            queue: Mutex::new(LinkedList::new()),
            current_interrupt_priority: Mutex::new(usize::MAX),
        }
    }

    #[inline(always)]
    pub fn push(&self, descriptor: PendingInterruptHandlerDescriptor) {
        if self.can_preempt(descriptor.priority) {
            self.queue.lock().unwrap().push_front(descriptor);
        } else {
            self.queue.lock().unwrap().push_back(descriptor);
        }
    }

    #[inline(always)]
    fn can_preempt(&self, priority: usize) -> bool {
        *self.current_interrupt_priority.lock().unwrap() > priority
    }
}

pub struct InterruptController<'a> {
    peripheral_ic: PeripheralIC<'a>,
    local_interrupt_controller: LocalInterruptController<'a>,
    pending_interrupt_queue: &'a PendingInterruptQueue,
}

impl<'a> InterruptController<'a> {
    pub const MAX_LOCAL_INTERRUPT_NUMBER: usize = 3;
    pub const MAX_PERIPHERAL_INTERRUPT_NUMBER: usize = 63;

    pub const unsafe fn new(
        peripherial_ic_mmio_start_addr: usize,
        core_interrupt_source_mmio_start_addr: usize,
        pending_interrupt_queue: &'a PendingInterruptQueue,
    ) -> Self {
        Self {
            peripheral_ic: PeripheralIC::new(
                peripherial_ic_mmio_start_addr,
                pending_interrupt_queue,
            ),
            local_interrupt_controller: LocalInterruptController::new(
                core_interrupt_source_mmio_start_addr,
                pending_interrupt_queue,
            ),
            pending_interrupt_queue,
        }
    }

    fn check_and_do_interrupt(&self) {
        if self
            .pending_interrupt_queue
            .queue
            .lock()
            .unwrap()
            .is_empty()
        {
            return;
        }
        let next_priority = self
            .pending_interrupt_queue
            .queue
            .lock()
            .unwrap()
            .front()
            .unwrap()
            .priority;
        if !self.pending_interrupt_queue.can_preempt(next_priority) {
            return;
        }
        while let Some(descriptor) = self
            .pending_interrupt_queue
            .queue
            .lock()
            .unwrap()
            .pop_front()
        {
            *self
                .pending_interrupt_queue
                .current_interrupt_priority
                .lock()
                .unwrap() = descriptor.priority;
            unsafe { enable_kernel_space_interrupt() };
            descriptor.handler.handle().unwrap();
            unsafe { disable_kernel_space_interrupt() };
            *self
                .pending_interrupt_queue
                .current_interrupt_priority
                .lock()
                .unwrap() = usize::MAX;
        }
        unsafe { enable_kernel_space_interrupt() };
    }
}

impl<'a> DeviceDriver for InterruptController<'a> {
    type InterruptNumberType = InterruptNumber;
}

impl<'a> InterruptManager for InterruptController<'a> {
    type InterruptNumberType = InterruptNumber;

    fn register_handler(
        &self,
        interrupt_handler_descriptor: InterruptHandlerDescriptor<Self::InterruptNumberType>,
    ) -> Result<(), &'static str> {
        match interrupt_handler_descriptor.number() {
            InterruptNumber::Local(local_interrupt_number) => self
                .local_interrupt_controller
                .register_handler(InterruptHandlerDescriptor::new(
                    local_interrupt_number,
                    interrupt_handler_descriptor.name(),
                    interrupt_handler_descriptor.prehook(),
                    interrupt_handler_descriptor.handler(),
                    interrupt_handler_descriptor.priority(),
                )),
            InterruptNumber::Peripheral(peripheral_interrupt_number) => self
                .peripheral_ic
                .register_handler(InterruptHandlerDescriptor::new(
                    peripheral_interrupt_number,
                    interrupt_handler_descriptor.name(),
                    interrupt_handler_descriptor.prehook(),
                    interrupt_handler_descriptor.handler(),
                    interrupt_handler_descriptor.priority(),
                )),
        }
    }

    fn enable(&self, interrupt_number: &Self::InterruptNumberType) {
        match interrupt_number {
            InterruptNumber::Local(n) => self.local_interrupt_controller.enable(n),
            InterruptNumber::Peripheral(n) => self.peripheral_ic.enable(n),
        };
    }

    fn handle_pending_interrupt(&self) {
        self.local_interrupt_controller.handle_pending_interrupt();
        self.peripheral_ic.handle_pending_interrupt();
        self.check_and_do_interrupt();
    }
}
