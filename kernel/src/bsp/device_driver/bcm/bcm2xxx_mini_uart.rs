// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

//! MINI UART driver.
//!
//! # Resources
//!
//! - <https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf>
//! - <https://developer.arm.com/documentation/ddi0183/latest>

use crate::{
    bsp::device_driver::common::MMIODerefWrapper, console, cpu, driver, synchronization,
    synchronization::NullLock,
};
use aarch64_cpu::asm;
use core::fmt;
use tock_registers::{
    interfaces::{Readable, Writeable},
    register_bitfields, register_structs,
    registers::{ReadOnly, ReadWrite},
};

//--------------------------------------------------------------------------------------------------
// Private Definitions
//--------------------------------------------------------------------------------------------------

// MINI UART registers.
//
// Descriptions taken from "PrimeCell UART (MINI) Technical Reference Manual" r1p5.

register_bitfields! {
  u32,
  /// Auxiliary peripherals Register
  AUXIRQ [
    MINI_UART_IRQ OFFSET(0) NUMBITS(1) []
  ],

  /// Auxiliary enables
  AUXENB [
    MINI_UART_ENABLE OFFSET(0) NUMBITS(1) [
      Disable = 0,
      Enable = 1
    ]
  ],

  /// Mini Uart I/O Data
  AUX_MU_IO_REG [
    /// LS 8 bits Baudrate read/write, DLAB=1
    /// Transmit data write, DLAB=0
    /// Receive data read, DLAB=0
    DATA OFFSET(0) NUMBITS(8) []
  ],


  /// Mini Uart Interrupt Enable
  AUX_MU_IER_REG [
    RX OFFSET(0) NUMBITS(1) [
      Disable = 0,
      Enable = 1
    ],

    TX OFFSET(1) NUMBITS(1) [
      Disable = 0,
      Enable = 1
    ]
  ],

  /// Mini Uart Interrupt Identify
  AUX_MU_IIR_REG [
    /// This bit is clear whenever an interrupt is pending
    INTERRUPT_PENDING OFFSET(0) NUMBITS(1) [
      Disable = 0,
      Enable = 1
    ],

    /// On read this register shows the interrupt ID bit
    /// 00 : No interrupts
    /// 01 : Transmit holding register empty
    /// 10 : Receiver holds valid byte
    /// 11 : <Not possible>
    /// On write:
    /// Writing with bit 1 set will clear the receive FIFO
    /// Writing with bit 2 set will clear the transmit FIFO
    INTERRUPT_ID OFFSET(1) NUMBITS(2) [
      NO_INTERRUPT = 0b00,
      TX_HOLD = 0b01,
      RX_HOLD = 0b10,
      NOT_POSSIBLE = 0b11
    ]
  ],

  // Mini Uart Line Control
  AUX_MU_LCR_REG [
    DATA_SIZE OFFSET(0) NUMBITS(2) [
      EIGHT_BIT = 0b00,
      SEVEN_BIT = 0b11
    ]
  ],

  // Mini Uart Modem Control
  AUX_MU_MCR_REG [
    RTS OFFSET(1) NUMBITS(1) [
      Disable = 0,
      Enable = 1
    ]
  ],

  // Mini Uart Line Status
  AUX_MU_LSR_REG [
    DATA_READY OFFSET(0) NUMBITS(1) [
      No = 0,
      Yes = 1
    ],
    TX_EMPTY OFFSET(5) NUMBITS(1) [
      No = 0,
      Yes = 1
    ],
    TX_IDLE OFFSET(6) NUMBITS(1) [
      No = 0,
      Yes = 1
    ],
  ],

  /// Mini Uart Extra Control
  AUX_MU_CNTL_REG [
    ALL OFFSET(0) NUMBITS(8)
  ],

  /// Mini Uart Baudrate
  AUX_MU_BAUD_REG [
    BUAD_RATE OFFSET(0) NUMBITS(16) [
      Default = 270
    ]
  ]
}

register_structs! {
  #[allow(non_snake_case)]
  pub RegisterBlock {
      (0x00 => AUXIRQ: ReadOnly<u32, AUXIRQ::Register>),
      (0x04 => AUXENB: ReadWrite<u32, AUXENB::Register>),
      (0x08 => _reserved1),
      (0x40 => AUX_MU_IO_REG: ReadWrite<u32>),
      (0x44 => AUX_MU_IER_REG: ReadWrite<u32, AUX_MU_IER_REG::Register>),
      (0x48 => AUX_MU_IIR_REG: ReadWrite<u32, AUX_MU_IIR_REG::Register>),
      (0x4c => AUX_MU_LCR_REG: ReadWrite<u32, AUX_MU_LCR_REG::Register>),
      (0x50 => AUX_MU_MCR_REG: ReadWrite<u32, AUX_MU_MCR_REG::Register>),
      (0x54 => AUX_MU_LSR_REG: ReadOnly<u32, AUX_MU_LSR_REG::Register>),
      (0x58 => _reserved2),
      (0x60 => AUX_MU_CNTL_REG: ReadWrite<u32, AUX_MU_CNTL_REG::Register>),
      (0x64 => _reserved3),
      (0x68 => AUX_MU_BAUD_REG: ReadWrite<u32, AUX_MU_BAUD_REG::Register>),
      (0x6C => @END),
  }
}

/// Abstraction for the associated MMIO registers.
type Registers = MMIODerefWrapper<RegisterBlock>;

#[derive(PartialEq)]
enum BlockingMode {
    Blocking,
    NonBlocking,
}

struct MiniUartInner {
    registers: Registers,
    chars_written: usize,
    chars_read: usize,
}

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

/// Representation of the UART.
pub struct MiniUart {
    inner: NullLock<MiniUartInner>,
}

//--------------------------------------------------------------------------------------------------
// Private Code
//--------------------------------------------------------------------------------------------------

impl MiniUartInner {
    /// Create an instance.
    ///
    /// # Safety
    ///
    /// - The user must ensure to provide a correct MMIO start address.
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: Registers::new(mmio_start_addr),
            chars_written: 0,
            chars_read: 0,
        }
    }

    /// Set up baud rate and characteristics.
    pub fn init(&mut self) {
        // Set AUXENB register to enable mini UART. Then mini UART register can be accessed.
        self.registers.AUXENB.set(1);

        // Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
        // Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
        // Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
        // Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
        // Set AUX_MU_BAUD to 270. Set baud rate to 115200
        // After booting, the system clock is 250 MHz.
        // Set AUX_MU_IIR_REG to 6.
        // Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
        self.registers.AUX_MU_CNTL_REG.set(0);
        self.registers.AUX_MU_IER_REG.set(0);
        self.registers.AUX_MU_LCR_REG.set(3);
        self.registers.AUX_MU_MCR_REG.set(0);
        self.registers
            .AUX_MU_BAUD_REG
            .write(AUX_MU_BAUD_REG::BUAD_RATE::Default);
        // self.registers.AUX_MU_IIR_REG.set(0xc6); //todo NOT_POSSIBLE
        self.registers
            .AUX_MU_IIR_REG
            .write(AUX_MU_IIR_REG::INTERRUPT_ID::NOT_POSSIBLE); //todo NOT_POSSIBLE

        self.registers.AUX_MU_CNTL_REG.set(3);
    }

    /// Send a character.
    fn write_char(&mut self, c: char) {
        // wait while input
        while self
            .registers
            .AUX_MU_LSR_REG
            .matches_all(AUX_MU_LSR_REG::TX_IDLE::No)
        {
            asm::nop();
        }

        // Write the character to the buffer.
        self.registers.AUX_MU_IO_REG.set(c as u32);

        self.chars_written += 1;
    }

    /// Block execution until the last buffered character has been physically put on the TX wire.
    fn flush(&self) {
        // Spin until the busy bit is cleared.
        while self
            .registers
            .AUX_MU_LSR_REG
            .matches_all(AUX_MU_LSR_REG::TX_IDLE::No)
        {
            cpu::nop();
        }
    }

    /// Retrieve a character.
    fn read_char_converting(&mut self, blocking_mode: BlockingMode) -> Option<char> {
        // If RX FIFO is empty,
        if self
            .registers
            .AUX_MU_LSR_REG
            .matches_all(AUX_MU_LSR_REG::DATA_READY::No)
        {
            // immediately return in non-blocking mode.
            if blocking_mode == BlockingMode::NonBlocking {
                return None;
            }

            // Otherwise, wait until a char was received.
            while self
                .registers
                .AUX_MU_LSR_REG
                .matches_all(AUX_MU_LSR_REG::DATA_READY::No)
            {
                cpu::nop();
            }
        }

        // Otherwise, wait until a char was received.
        while self
            .registers
            .AUX_MU_LSR_REG
            .matches_all(AUX_MU_LSR_REG::DATA_READY::No)
        {
            cpu::nop();
        }

        // Read one character.
        let mut ret = self.registers.AUX_MU_IO_REG.get() as u8 as char;

        // Convert carrige return to newline.
        if ret == '\r' {
            ret = '\n';
        }

        // Update statistics.
        self.chars_read += 1;

        Some(ret)
    }
}

/// Implementing `core::fmt::Write` enables usage of the `format_args!` macros, which in turn are
/// used to implement the `kernel`'s `print!` and `println!` macros. By implementing `write_str()`,
/// we get `write_fmt()` automatically.
///
/// The function takes an `&mut self`, so it must be implemented for the inner struct.
///
/// See [`src/print.rs`].
///
/// [`src/print.rs`]: ../../print/index.html
impl fmt::Write for MiniUartInner {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for c in s.chars() {
            self.write_char(c);
        }

        Ok(())
    }
}

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

impl MiniUart {
    pub const COMPATIBLE: &'static str = "MINI UART";

    /// Create an instance.
    ///
    /// # Safety
    ///
    /// - The user must ensure to provide a correct MMIO start address.
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: NullLock::new(MiniUartInner::new(mmio_start_addr)),
        }
    }
}

//------------------------------------------------------------------------------
// OS Interface Code
//------------------------------------------------------------------------------
use synchronization::interface::Mutex;

impl driver::interface::DeviceDriver for MiniUart {
    fn compatible(&self) -> &'static str {
        Self::COMPATIBLE
    }

    unsafe fn init(&self) -> Result<(), &'static str> {
        self.inner.lock(|inner| inner.init());

        Ok(())
    }
}

impl console::interface::Write for MiniUart {
    /// Passthrough of `args` to the `core::fmt::Write` implementation, but guarded by a Mutex to
    /// serialize access.
    fn write_char(&self, c: char) {
        self.inner.lock(|inner| inner.write_char(c));
    }

    fn write_fmt(&self, args: core::fmt::Arguments) -> fmt::Result {
        // Fully qualified syntax for the call to `core::fmt::Write::write_fmt()` to increase
        // readability.
        self.inner.lock(|inner| fmt::Write::write_fmt(inner, args))
    }

    fn flush(&self) {
        // Spin until TX FIFO empty is set.
        self.inner.lock(|inner| inner.flush());
    }
}

impl console::interface::Read for MiniUart {
    fn read_char(&self) -> char {
        self.inner
            .lock(|inner| inner.read_char_converting(BlockingMode::Blocking).unwrap())
    }

    fn clear_rx(&self) {
        // Read from the RX FIFO until it is indicating empty.
        while self
            .inner
            .lock(|inner| inner.read_char_converting(BlockingMode::NonBlocking))
            .is_some()
        {}
    }
}

impl console::interface::Statistics for MiniUart {
    fn chars_written(&self) -> usize {
        self.inner.lock(|inner| inner.chars_written)
    }

    fn chars_read(&self) -> usize {
        self.inner.lock(|inner| inner.chars_read)
    }
}

impl console::interface::All for MiniUart {}
