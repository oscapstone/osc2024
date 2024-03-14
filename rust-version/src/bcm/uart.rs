// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

//! PL011 UART driver.
//!
//! # Resources
//!
//! - <https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf>
//! - <https://developer.arm.com/documentation/ddi0183/latest>

use crate::bcm::common::MMIODerefWrapper;
use crate::synchronization::{interface::Mutex, NullLock};
use aarch64_cpu::asm;
use core::char;

use tock_registers::{
    interfaces::Writeable,
    register_bitfields, register_structs,
    registers::{ReadOnly, ReadWrite},
};

use core::fmt;
use tock_registers::interfaces::Readable;

//--------------------------------------------------------------------------------------------------
// Public Definitions
// -------------------------------------------------------------------------------------------------

pub struct Uart {
    inner: NullLock<UartInner>,
}

//--------------------------------------------------------------------------------------------------
// Private Definitions
//--------------------------------------------------------------------------------------------------

// mini UART registers.
register_bitfields! {
    u32,
    AUX_ENABLES [
        SPI2_ENABLE OFFSET(2) NUMBITS(1) [
        ],
        SPI1_ENABLE OFFSET(1) NUMBITS(1) [
        ],
        MINI_UART_ENABLE OFFSET(0) NUMBITS(1) [
            Enable = 1,
            Disable = 0
        ]
    ],
    AUX_MU_IO [
        DATA OFFSET(0) NUMBITS(8) [],
    ],
    AUX_MU_IER [
        ENABLE_TX OFFSET(1) NUMBITS(1) [ ],
        ENABLE_RX OFFSET(0) NUMBITS(1) [ ],
    ],
    AUX_MU_IIR [
        RW_INT_ID OFFSET(1) NUMBITS(2) [
            NoInterrupt = 0b00,
            TransmitterEmpty = 0b01,
            ReceiverDataAvailable = 0b10,
            DisableInterrupt = 0b11
        ],
    ],

    AUX_MU_LCR [
        DATA_SIZE OFFSET(0) NUMBITS(2) [
            SevenBit = 0b00,
            EightBit = 0b11
        ],
    ],
    AUX_MU_MCR [
        AUTO_FLOW_RTS OFFSET(1) NUMBITS(1) [
            Inactive = 0,
            Active = 1
        ],
    ],
    AUX_MU_LSR [
        RX_READY OFFSET(0) NUMBITS(1) [],
        TX_IDLE OFFSET(6) NUMBITS(1) [],
    ],
    AUX_MU_CNTL [
        TX_RX_ENABLE OFFSET(0) NUMBITS(2) [
            rxTxDisable = 0b00,
            rxEnable = 0b01,
            txEnable = 0b10,
            rxTxEnable = 0b11,
        ],
    ],
    AUX_MU_BAUD [
        Baudrate OFFSET(0) NUMBITS(16) [],
    ],
}

register_structs! {
    #[allow(non_snake_case)]
    pub RegisterBlock {
        (0x00 => _reserved1),
        (0x04 => AUX_ENABLES: ReadWrite<u32, AUX_ENABLES::Register>),
        (0x08 => _reserved2),
        (0x40 => AUX_MU_IO: ReadWrite<u32>),
        (0x44 => AUX_MU_IER: ReadWrite<u32, AUX_MU_IER::Register>),
        (0x48 => AUX_MU_IIR: ReadWrite<u32, AUX_MU_IIR::Register>),
        (0x4c => AUX_MU_LCR: ReadWrite<u32, AUX_MU_LCR::Register>),
        (0x50 => AUX_MU_MCR: ReadWrite<u32, AUX_MU_MCR::Register>),
        (0x54 => AUX_MU_LSR: ReadOnly<u32, AUX_MU_LSR::Register>),
        (0x58 => _reserved3),
        (0x60 => AUX_MU_CNTL: ReadWrite<u32, AUX_MU_CNTL::Register>),
        (0x64 => _reserved4),
        (0x68 => AUX_MU_BAUD: ReadWrite<u32, AUX_MU_BAUD::Register>),
        (0x6c => _reserved5),
        (0xd4 => @END),
    }
}

/// Abstraction for the associated MMIO registers.
type Registers = MMIODerefWrapper<RegisterBlock>;

struct UartInner {
    registers: Registers,
}

//--------------------------------------------------------------------------------------------------
// Private Code
//--------------------------------------------------------------------------------------------------

impl UartInner {
    const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: Registers::new(mmio_start_addr),
        }
    }

    fn init(&mut self) {
        // Enable the mini UART.
        self.registers
            .AUX_ENABLES
            .write(AUX_ENABLES::MINI_UART_ENABLE::Enable);

        // disable transmitter and receiver
        self.registers
            .AUX_MU_CNTL
            .write(AUX_MU_CNTL::TX_RX_ENABLE::rxTxDisable);

        // set data size to 8 bit
        self.registers
            .AUX_MU_LCR
            .write(AUX_MU_LCR::DATA_SIZE::EightBit);

        // don't need auto flow control
        self.registers
            .AUX_MU_MCR
            .write(AUX_MU_MCR::AUTO_FLOW_RTS::Inactive);

        // disable interrupts
        self.registers
            .AUX_MU_IER
            .write(AUX_MU_IER::ENABLE_RX::CLEAR + AUX_MU_IER::ENABLE_RX::CLEAR);

        self.registers
            .AUX_MU_IIR
            .write(AUX_MU_IIR::RW_INT_ID::DisableInterrupt);

        // set baudrate to 115200
        self.registers
            .AUX_MU_BAUD
            .write(AUX_MU_BAUD::Baudrate.val(270));

        self.registers
            .AUX_MU_CNTL
            .write(AUX_MU_CNTL::TX_RX_ENABLE::rxEnable + AUX_MU_CNTL::TX_RX_ENABLE::txEnable);
    }

    fn get_char(&self) -> char {
        // wait until transmitter is empty
        while self
            .registers
            .AUX_MU_LSR
            .matches_all(AUX_MU_LSR::RX_READY::CLEAR)
        {
            asm::nop();
        }

        char::from_u32(self.registers.AUX_MU_IO.get()).unwrap()
    }
}

impl core::fmt::Write for UartInner {
    fn write_char(&mut self, c: char) -> fmt::Result {
        // wait until transmitter is empty
        while self
            .registers
            .AUX_MU_LSR
            .matches_all(AUX_MU_LSR::TX_IDLE::CLEAR)
        {
            asm::nop();
        }
        self.registers.AUX_MU_IO.set(c as u32);
        Ok(())
    }

    fn write_str(&mut self, s: &str) -> fmt::Result {
        for c in s.chars() {
            self.write_char(c);
        }

        Ok(())
    }
}

//--------------------------------------------------------------------------------------------------
// Public Code
// -------------------------------------------------------------------------------------------------

impl Uart {
    // TODO: add loader support
    // pub const COMPATIBLE: &'static str = "BCM Mini UART";

    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: NullLock::new(UartInner::new(mmio_start_addr)),
        }
    }

    pub fn init(&self) {
        self.inner.lock(|inner| inner.init());
    }

    pub fn get_char(&self) -> char {
        self.inner.lock(|inner| inner.get_char())
    }
}

use crate::console::interface;
impl interface::Write for Uart {
    fn write_fmt(&self, args: core::fmt::Arguments) -> fmt::Result {
        // Fully qualified syntax for the call to `core::fmt::Write::write_fmt()` to increase
        // readability.
        self.inner.lock(|inner| fmt::Write::write_fmt(inner, args))
    }
}
