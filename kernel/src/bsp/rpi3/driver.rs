// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

//! BSP driver support.

use super::memory::map::mmio;
use crate::{bsp::device_driver, console, mbox, driver as generic_driver};
use core::sync::atomic::{AtomicBool, Ordering};

//--------------------------------------------------------------------------------------------------
// Global instances
//--------------------------------------------------------------------------------------------------

static MINI_UART: device_driver::MiniUart =
    unsafe { device_driver::MiniUart::new(mmio::MINI_UART_START) };
// static PL011_UART: device_driver::PL011Uart =
//     unsafe { device_driver::PL011Uart::new(mmio::PL011_UART_START) };
static GPIO: device_driver::GPIO = unsafe { device_driver::GPIO::new(mmio::GPIO_START) };
pub static MBOX: device_driver::MBOX =
    unsafe { device_driver::MBOX::new(mmio::MAILBOX_START) };
    // unsafe { device_driver::MBOX::new(mmio::MAILBOX_START) };

//--------------------------------------------------------------------------------------------------
// Private Code
//--------------------------------------------------------------------------------------------------

/// This must be called only after successful init of the UART driver.
fn post_init_uart() -> Result<(), &'static str> {
    // console::register_console(&PL011_UART);
    console::register_console(&MINI_UART);

    Ok(())
}

/// This must be called only after successful init of the GPIO driver.
fn post_init_gpio() -> Result<(), &'static str> {
    // GPIO.map_pl011_uart();
    GPIO.map_mini_uart();
    Ok(())
}


fn post_init_mbox() -> Result<(), &'static str> {
    mbox::register_mbox(&MBOX);
    Ok(())
}

fn driver_uart() -> Result<(), &'static str> {
    let uart_descriptor =
        // generic_driver::DeviceDriverDescriptor::new(&PL011_UART, Some(post_init_uart));
        generic_driver::DeviceDriverDescriptor::new(&MINI_UART, Some(post_init_uart));
    generic_driver::driver_manager().register_driver(uart_descriptor);

    Ok(())
}

fn driver_gpio() -> Result<(), &'static str> {
    let gpio_descriptor = generic_driver::DeviceDriverDescriptor::new(&GPIO, Some(post_init_gpio));
    generic_driver::driver_manager().register_driver(gpio_descriptor);

    Ok(())
}

fn driver_mbox() -> Result<(), &'static str> {
    let mbox_descriptor = generic_driver::DeviceDriverDescriptor::new(&MBOX, Some(post_init_mbox));
    generic_driver::driver_manager().register_driver(mbox_descriptor);
    
    Ok(())
}

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

/// Initialize the driver subsystem.
///
/// # Safety
///
/// See child function calls.
pub unsafe fn init() -> Result<(), &'static str> {
    static INIT_DONE: AtomicBool = AtomicBool::new(false);
    if INIT_DONE.load(Ordering::Relaxed) {
        return Err("Init already done");
    }
    
    driver_gpio()?;
    driver_uart()?;
    driver_mbox()?;
    

    INIT_DONE.store(true, Ordering::Relaxed);
    Ok(())
}