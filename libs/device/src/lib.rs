#![no_std]

extern crate alloc;

pub mod common;
pub mod device_driver;
pub mod gpio;
pub mod interrupt_controller;
pub mod interrupt_manager;
pub mod local_interrupt_controller;
pub mod mailbox;
pub mod mini_uart;
pub mod peripheral_ic;
pub mod timer;
pub mod watchdog;
