use bsp::memory::{
    AUX_MMIO_BASE, CORE_INTERRUPT_SOURCE_MMIO_BASE, CORE_TIMER_INTERRUPT_CONTROLL_MMIO_BASE,
    GPIO_MMIO_BASE, INTERRUPT_CONTROLLER_MMIO_BASE, MAILBOX_MMIO_BASE, WATCHDOG_MMIO_BASE,
};
use device::device_driver::{driver_manager, DeviceDriverDescriptor};
use device::gpio::GPIO;
use device::interrupt_controller::{
    InterruptController, InterruptNumber, LocalInterrupt, LocalInterruptType,
    PendingInterruptQueue, PeripheralInterrupt, PeripherialInterruptType,
};
use device::interrupt_manager::register_interrupt_manager;
use device::mailbox::Mailbox;
use device::mini_uart::MiniUart;
use device::timer::Timer;
use device::watchdog::Watchdog;
use library::console;

use crate::memory::phys_to_virt;

static MINI_UART: MiniUart = unsafe { MiniUart::new(phys_to_virt(AUX_MMIO_BASE)) };
static GPIO: GPIO = unsafe { GPIO::new(phys_to_virt(GPIO_MMIO_BASE)) };
static WATCHDOG: Watchdog = unsafe { Watchdog::new(phys_to_virt(WATCHDOG_MMIO_BASE)) };
static MAILBOX: Mailbox = unsafe { Mailbox::new(phys_to_virt(MAILBOX_MMIO_BASE)) };
static PENDING_INTERRUPT_QUEUE: PendingInterruptQueue = PendingInterruptQueue::new();
static INTERRUPT_CONTROLLER: InterruptController = unsafe {
    InterruptController::new(
        phys_to_virt(INTERRUPT_CONTROLLER_MMIO_BASE),
        phys_to_virt(CORE_INTERRUPT_SOURCE_MMIO_BASE),
        &PENDING_INTERRUPT_QUEUE,
    )
};
static TIMER: Timer = unsafe { Timer::new(phys_to_virt(CORE_TIMER_INTERRUPT_CONTROLL_MMIO_BASE)) };

pub unsafe fn init() -> Result<(), &'static str> {
    let driver_manager = driver_manager();
    driver_manager.register_driver(DeviceDriverDescriptor::new(
        &GPIO,
        Some(|| {
            GPIO.setup_for_mini_uart();
            Ok(())
        }),
        None,
    ));
    driver_manager.register_driver(DeviceDriverDescriptor::new(
        &MINI_UART,
        Some(|| {
            console::register_console(&MINI_UART);
            Ok(())
        }),
        Some(InterruptNumber::Peripheral(PeripheralInterrupt::new(
            PeripherialInterruptType::Aux as usize,
        ))),
    ));
    driver_manager.register_driver(DeviceDriverDescriptor::new(&WATCHDOG, None, None));
    driver_manager.register_driver(DeviceDriverDescriptor::new(&MAILBOX, None, None));
    driver_manager.register_driver(DeviceDriverDescriptor::new(
        &TIMER,
        None,
        Some(InterruptNumber::Local(LocalInterrupt::new(
            LocalInterruptType::Timer1 as usize,
        ))),
    ));
    driver_manager.register_driver(DeviceDriverDescriptor::new(
        &INTERRUPT_CONTROLLER,
        Some(|| {
            register_interrupt_manager(&INTERRUPT_CONTROLLER);
            Ok(())
        }),
        None,
    ));
    driver_manager.init_drivers_and_interrupts();

    Ok(())
}

pub fn watchdog() -> &'static Watchdog {
    &WATCHDOG
}

pub fn mailbox() -> &'static Mailbox {
    &MAILBOX
}

pub fn timer() -> &'static Timer {
    &TIMER
}
