use core::sync::atomic::{AtomicBool, Ordering};

use device::{
    driver::DeviceDriverDescriptor, gpio::GPIO, mailbox::Mailbox, mini_uart::MiniUart,
    watchdog::Watchdog,
};
use small_std::fmt::print::console;

pub const PERIPHERAL_MMIO_BASE: usize = 0x3f000000;
pub const GPIO_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x00200000;
pub const AUX_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x00215000;
pub const WATCHDOG_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x00100000;
pub const MAILBOX_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x0000b880;

static GPIO: GPIO = unsafe { GPIO::new(GPIO_MMIO_BASE) };
static MINI_UART: MiniUart = unsafe { MiniUart::new(AUX_MMIO_BASE) };
static WATCHDOG: Watchdog = unsafe { Watchdog::new(WATCHDOG_MMIO_BASE) };
static MAILBOX: Mailbox = unsafe { Mailbox::new(MAILBOX_MMIO_BASE) };

pub unsafe fn register_drivers() -> Result<(), &'static str> {
    static INIT_DONE: AtomicBool = AtomicBool::new(false);
    if INIT_DONE.load(Ordering::Relaxed) {
        return Err("Init already done");
    }

    let driver_manager = device::driver::driver_manager();

    let gpio = DeviceDriverDescriptor::new(&GPIO, Some(gpio_post_init));
    driver_manager.register_driver(gpio);

    let mini_uart = DeviceDriverDescriptor::new(&MINI_UART, Some(mini_uart_post_init));
    driver_manager.register_driver(mini_uart);

    let watchdog = DeviceDriverDescriptor::new(&WATCHDOG, None);
    driver_manager.register_driver(watchdog);

    let mailbox = DeviceDriverDescriptor::new(&MAILBOX, None);
    driver_manager.register_driver(mailbox);

    Ok(())
}

fn gpio_post_init() -> Result<(), &'static str> {
    GPIO.map_mini_uart();
    Ok(())
}

fn mini_uart_post_init() -> Result<(), &'static str> {
    console::register_console(&MINI_UART);
    Ok(())
}

pub fn watchdog() -> &'static Watchdog {
    &WATCHDOG
}

pub fn mailbox() -> &'static Mailbox {
    &MAILBOX
}
