use library::sync::mutex::Mutex;
use tock_registers::{
    interfaces::Readable, register_bitfields, register_structs, registers::ReadOnly,
};

use crate::{
    common::MMIODerefWrapper,
    interrupt_controller::{
        LocalInterrupt, PendingInterruptHandlerDescriptor, PendingInterruptQueue,
    },
    interrupt_manager::{InterruptHandlerDescriptor, InterruptManager},
};

register_bitfields! [
    u32,
    INTERRUPT_SOURCE [
        LOCAL_TIMER_INTERRUPT OFFSET(11) NUMBITS(1) [],
        AXI_OUTSTANDING_INTERRUPT OFFSET(10) NUMBITS(1) [],
        PMU_INTERRUPT OFFSET(9) NUMBITS(1) [],
        GPU_INTERRUPT OFFSET(8) NUMBITS(1) [],
        MAILBOX_3_INTERRUPT OFFSET(7) NUMBITS(1) [],
        MAILBOX_2_INTERRUPT OFFSET(6) NUMBITS(1) [],
        MAILBOX_1_INTERRUPT OFFSET(5) NUMBITS(1) [],
        MAILBOX_0_INTERRUPT OFFSET(4) NUMBITS(1) [],
        CNTVIRQ_INTERRUPT OFFSET(3) NUMBITS(1) [],
        CNTHPIRQ_INTERRUPT OFFSET(2) NUMBITS(1) [],
        CNTPNSIRQ_INTERRUPT OFFSET(1) NUMBITS(1) [],
        CNTPSIRQ_INTERRUPT OFFSET(0) NUMBITS(1) [],
    ]
];

register_structs! {
    CoreInterruptSourceRegisters {
        (0x00 => core0_interrupt_source: ReadOnly<u32, INTERRUPT_SOURCE::Register>),
        (0x04 => core1_interrupt_source: ReadOnly<u32, INTERRUPT_SOURCE::Register>),
        (0x08 => core2_interrupt_source: ReadOnly<u32, INTERRUPT_SOURCE::Register>),
        (0x0c => core3_interrupt_source: ReadOnly<u32, INTERRUPT_SOURCE::Register>),
        (0x10 => @END),
    }
}

type HandlerTable = [Option<InterruptHandlerDescriptor<LocalInterrupt>>; LocalInterrupt::HIGH];

pub struct LocalInterruptController<'a> {
    readonly_registers: MMIODerefWrapper<CoreInterruptSourceRegisters>,
    handlers: Mutex<HandlerTable>,
    pending_interrupt_queue: &'a PendingInterruptQueue,
}

impl<'a> LocalInterruptController<'a> {
    pub const unsafe fn new(
        mmio_start_addr: usize,
        pending_interrupt_queue: &'a PendingInterruptQueue,
    ) -> Self {
        Self {
            readonly_registers: MMIODerefWrapper::new(mmio_start_addr),
            handlers: Mutex::new([None; LocalInterrupt::HIGH]),
            pending_interrupt_queue,
        }
    }
}

impl<'a> InterruptManager for LocalInterruptController<'a> {
    type InterruptNumberType = LocalInterrupt;

    fn register_handler(
        &self,
        interrupt_handler_descriptor: InterruptHandlerDescriptor<Self::InterruptNumberType>,
    ) -> Result<(), &'static str> {
        let interrupt_number = *interrupt_handler_descriptor.number();
        let mut handlers = self.handlers.lock().unwrap();
        if handlers[interrupt_number].is_some() {
            return Err("Handler already registered.");
        }
        handlers[interrupt_number] = Some(interrupt_handler_descriptor);
        Ok(())
    }

    fn enable(&self, _: &Self::InterruptNumberType) {}

    fn handle_pending_interrupt(&self) {
        let mut bitmask = self.readonly_registers.core0_interrupt_source.get();
        for interrupt_number in 0..4 {
            if bitmask & 1 == 1 {
                match self.handlers.lock().unwrap()[interrupt_number] {
                    None => panic!("No handler registered for interrupt {}", interrupt_number),
                    Some(descriptor) => {
                        if let Some(prehook) = descriptor.prehook() {
                            prehook.prehook().unwrap();
                        }
                        self.pending_interrupt_queue
                            .push(PendingInterruptHandlerDescriptor::new(
                                descriptor.handler(),
                                descriptor.priority(),
                            ))
                    }
                }
            }
            bitmask >>= 1;
        }
    }
}
