use bsp::memory::{AUX_MMIO_BASE, GPIO_MMIO_BASE};
use device::device_driver::{driver_manager, DeviceDriverDescriptor};
use device::gpio::GPIO;
use device::interrupt_controller::{
    InterruptNumber, PeripheralInterrupt, PeripherialInterruptType,
};
use device::mini_uart::MiniUart;
use library::console;

static MINI_UART: MiniUart = unsafe { MiniUart::new(AUX_MMIO_BASE) };
static GPIO: GPIO = unsafe { GPIO::new(GPIO_MMIO_BASE) };

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
    driver_manager.init_drivers_and_interrupts();
    Ok(())
}
