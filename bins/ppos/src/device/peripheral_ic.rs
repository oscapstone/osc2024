use library::sync::mutex::Mutex;
use tock_registers::{
    interfaces::{Readable, Writeable},
    register_structs,
    registers::{ReadOnly, ReadWrite},
};

use super::{
    common::MMIODerefWrapper,
    interrupt_controller::{
        PendingInterruptHandlerDescriptor, PendingInterruptQueue, PendingInterrupts,
        PeripheralInterrupt,
    },
    interrupt_manager::{InterruptHandlerDescriptor, InterruptManager},
};

register_structs! {
    ReadOnlyRegisters {
        (0x00 => irq_basic_pending: ReadOnly<u32>),
        (0x04 => irq_pending_1: ReadOnly<u32>),
        (0x08 => irq_pending_2: ReadOnly<u32>),
        (0x0c => @END),
    }
}

register_structs! {
    ReadWriteRegisters {
        (0x00 => _reserved1),
        (0x0c => fiq_control: ReadWrite<u32>),
        (0x10 => enable_irqs_1: ReadWrite<u32>),
        (0x14 => enable_irqs_2: ReadWrite<u32>),
        (0x18 => enable_basic_irqs: ReadWrite<u32>),
        (0x1c => disable_irqs_1: ReadWrite<u32>),
        (0x20 => disable_irqs_2: ReadWrite<u32>),
        (0x24 => disable_basic_irqs: ReadWrite<u32>),
        (0x28 => @END),
    }
}

type HandlerTable =
    [Option<InterruptHandlerDescriptor<PeripheralInterrupt>>; PeripheralInterrupt::HIGH];

pub struct PeripheralIC<'a> {
    readonly_registers: MMIODerefWrapper<ReadOnlyRegisters>,
    readwrite_registers: Mutex<MMIODerefWrapper<ReadWriteRegisters>>,
    handlers: Mutex<HandlerTable>,
    pending_interrupt_queue: &'a PendingInterruptQueue,
}

impl<'a> PeripheralIC<'a> {
    pub const unsafe fn new(
        mmio_start_addr: usize,
        pending_interrupt_queue: &'a PendingInterruptQueue,
    ) -> Self {
        Self {
            readonly_registers: MMIODerefWrapper::new(mmio_start_addr),
            readwrite_registers: Mutex::new(MMIODerefWrapper::new(mmio_start_addr)),
            handlers: Mutex::new([None; PeripheralInterrupt::HIGH]),
            pending_interrupt_queue,
        }
    }

    fn pending_interrupts(&self) -> PendingInterrupts {
        let pending_mask: u64 = (u64::from(self.readonly_registers.irq_pending_2.get()) << 32)
            | u64::from(self.readonly_registers.irq_pending_1.get());
        PendingInterrupts::new(pending_mask)
    }
}

impl<'a> InterruptManager for PeripheralIC<'a> {
    type InterruptNumberType = PeripheralInterrupt;

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

    fn enable(&self, interrupt_number: &Self::InterruptNumberType) {
        let interrupt_number = **interrupt_number;
        let readwrite_registers = self.readwrite_registers.lock().unwrap();
        let reg = if interrupt_number <= 31 {
            &readwrite_registers.enable_irqs_1
        } else {
            &readwrite_registers.enable_irqs_2
        };
        let enable_bit: u32 = 1 << (interrupt_number % 32);
        reg.set(enable_bit);
    }

    fn handle_pending_interrupt(&self) {
        for interrupt_number in self.pending_interrupts() {
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
    }
}
