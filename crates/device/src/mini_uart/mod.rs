mod registers;

use core::fmt::Write;

use registers::{Registers, AUXENB, AUX_MU_CNTL, AUX_MU_IER, AUX_MU_IIR, AUX_MU_LCR, AUX_MU_LSR};
use small_std::{fmt::print::console, sync::Mutex};
use tock_registers::interfaces::{ReadWriteable, Readable, Writeable};

use crate::driver::DeviceDriver;

struct MiniUartInner {
    registers: Registers,
}

pub struct MiniUart {
    inner: Mutex<MiniUartInner>,
}

impl MiniUartInner {
    /// SAFETY: The user msut ensure to provide a correct MMIO start address
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: Registers::new(mmio_start_addr),
        }
    }

    fn init(&self) {
        self.registers.AUXENB.modify(AUXENB::MINI_UART::Enable);

        // disable transmitter and receiver during configuration
        self.registers
            .AUX_MU_CNTL
            .modify(AUX_MU_CNTL::TRANSMITTER::Disable + AUX_MU_CNTL::RECEIVER::Disable);

        // disable interrupt
        self.registers.AUX_MU_IER.modify(
            AUX_MU_IER::TRANSMIT_INTERRUPT::Disable + AUX_MU_IER::RECEIVE_INTERRUPT::Disable,
        );

        // set data size to be 8 bits
        self.registers
            .AUX_MU_LCR
            .modify(AUX_MU_LCR::DATA_SIZE::EightBits);

        // disable auto flow control
        self.registers.AUX_MU_MCR.set(0);

        // set baud rate to 115200
        self.registers.AUX_MU_BAUD.set(270);

        // disable FIFO
        self.registers.AUX_MU_IIR.modify(
            AUX_MU_IIR::FIFO_CLEAR_BITS::ClearTransmitFifo
                + AUX_MU_IIR::FIFO_CLEAR_BITS::ClearReceiveFifo,
        );

        // enable transmitter and receiver
        self.registers
            .AUX_MU_CNTL
            .modify(AUX_MU_CNTL::TRANSMITTER::Enable + AUX_MU_CNTL::RECEIVER::Enable);
    }

    /// Check if data is available to read.
    fn is_readable(&self) -> bool {
        self.registers.AUX_MU_LSR.is_set(AUX_MU_LSR::DATA_READY)
    }

    /// Check if data is available to write.
    fn is_writable(&self) -> bool {
        self.registers
            .AUX_MU_LSR
            .is_set(AUX_MU_LSR::TRANSMITTER_EMPTY)
    }

    pub fn read_byte(&self) -> u8 {
        while !self.is_readable() {}
        self.registers.AUX_MU_IO.get() as u8
    }

    pub fn write_byte(&self, byte: u8) {
        while !self.is_writable() {}
        self.registers.AUX_MU_IO.set(byte as u32)
    }

    fn flush(&self) {
        // Spin until the transmitter is empty
        while !self.is_writable() {}
    }
}

impl core::fmt::Write for MiniUartInner {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            if c == '\n' {
                self.write_byte(b'\r');
            }
            self.write_byte(c as u8);
        }
        Ok(())
    }
}

impl MiniUart {
    pub const COMPATIBLE: &'static str = "Mini UART";

    /// SAFETY: The user msut ensure to provide a correct MMIO start address
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: Mutex::new(MiniUartInner::new(mmio_start_addr)),
        }
    }
}

impl DeviceDriver for MiniUart {
    fn compatible(&self) -> &'static str {
        Self::COMPATIBLE
    }

    unsafe fn init(&self) -> Result<(), &'static str> {
        let inner = self.inner.lock().unwrap();
        inner.init();

        Ok(())
    }
}

impl console::Write for MiniUart {
    fn write_char(&self, c: char) {
        let inner = self.inner.lock().unwrap();
        if c == '\n' {
            inner.write_byte(b'\r');
        }
        inner.write_byte(c as u8);
    }

    fn write_fmt(&self, args: core::fmt::Arguments) -> core::fmt::Result {
        let mut inner = self.inner.lock().unwrap();
        inner.write_fmt(args)
    }

    fn flush(&self) {
        let inner = self.inner.lock().unwrap();
        inner.flush();
    }
}

impl console::Read for MiniUart {
    fn read_char(&self) -> char {
        let inner = self.inner.lock().unwrap();
        inner.read_byte() as char
    }

    fn clear_rx(&self) {
        let inner = self.inner.lock().unwrap();
        while inner.is_readable() {
            inner.read_byte();
        }
    }
}

impl console::All for MiniUart {}
