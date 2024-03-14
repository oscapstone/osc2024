// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2020-2023 Andre Richter <andre.o.richter@gmail.com>

//! Common device driver code.

use core::{marker::PhantomData, ops, ptr::write_volatile};

//--------------------------------------------------------------------------------------------------
// Private Definitions
// -------------------------------------------------------------------------------------------------

const PM_PASSWORD: u32 = 0x5a000000;
const PM_RSTC: u32 = 0x3F10_001C;
const PM_WDOG: u32 = 0x3F10_0024;

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

pub struct MMIODerefWrapper<T> {
    start_addr: usize,
    phantom: PhantomData<fn() -> T>,
}

/// The board's physical memory map.
#[rustfmt::skip]
pub mod map {

    pub const GPIO_OFFSET:         usize = 0x0020_0000;
    pub const UART_OFFSET:         usize = 0x0021_5000;
    pub const MAILBOX_OFFSET:      usize = 0x0000_B880;

    /// Physical devices.
    pub mod mmio {
        use super::*;

        pub const START:            usize =         0x3F00_0000;
        pub const GPIO_START:       usize = START + GPIO_OFFSET;
        pub const UART_START:       usize = START + UART_OFFSET;
        pub const MAILBOX_START:    usize = START + MAILBOX_OFFSET;
    }
}

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

impl<T> MMIODerefWrapper<T> {
    /// Create an instance.
    pub const unsafe fn new(start_addr: usize) -> Self {
        Self {
            start_addr,
            phantom: PhantomData,
        }
    }
}

impl<T> ops::Deref for MMIODerefWrapper<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { &*(self.start_addr as *const _) }
    }
}

pub fn spin_for_cycles(cycles: usize) {
    for _ in 0..cycles {
        aarch64_cpu::asm::nop();
    }
}

pub fn reset(tick: u32) {
    unsafe {
        let mut r = PM_PASSWORD | 0x20;
        write_volatile(PM_RSTC as *mut u32, r);
        r = PM_PASSWORD | tick;
        write_volatile(PM_WDOG as *mut u32, r);
    }
}

pub fn cancel_reset() {
    unsafe {
        let r = PM_PASSWORD;
        write_volatile(PM_RSTC as *mut u32, r);
        write_volatile(PM_WDOG as *mut u32, r);
    }
}

