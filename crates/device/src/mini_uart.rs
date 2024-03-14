use core::fmt::Write;

use small_std::{fmt::print::console, sync::Mutex};
use tock_registers::{
    interfaces::{ReadWriteable, Readable, Writeable},
    register_bitfields, register_structs,
    registers::{ReadOnly, ReadWrite},
};

use crate::{common::MMIODerefWrapper, device_driver::DeviceDriver};

register_bitfields! {
    u32,

    AUXENB  [
        SPI2 OFFSET(2) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        SPI1 OFFSET(1) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        MINI_UART OFFSET(0) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ]
    ],
    AUX_MU_IER [
        RECEIVE_INTERRUPT OFFSET(1) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        TRANSMIT_INTERRUPT OFFSET(0) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ]
    ],
    AUX_MU_IIR [
        FIFO_CLEAR_BITS OFFSET(1) NUMBITS(2) [
            ClearReceiveFifo = 0b01,
            ClearTransmitFifo = 0b10,
        ],
    ],
    AUX_MU_LCR [
        DATA_SIZE OFFSET(0) NUMBITS(1) [
            SevenBits = 0,
            EightBits = 1,
        ]
    ],
    AUX_MU_MCR [
        RTS OFFSET(1) NUMBITS(1) [
            High = 0,
            Low = 1,
        ]
    ],
    AUX_MU_LSR [
        TRANSMITTER_IDLE OFFSET(6) NUMBITS(1) [
            Idle = 1,
        ],
        TRANSMITTER_EMPTY OFFSET(5) NUMBITS(1) [
            Empty = 1,
        ],
        RECEIVER_OVERRUN OFFSET(1) NUMBITS(1) [
            Overrun = 1,
        ],
        DATA_READY OFFSET(0) NUMBITS(1) [
            Ready = 1,
        ]
    ],
    AUX_MU_CNTL [
        TRANSMIT_AUTO_FLOW_CONTROL OFFSET(3) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        RECEIVE_AUTO_FLOW_CONTROL OFFSET(2) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        TRANSMITTER OFFSET(1) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        RECEIVER OFFSET(0) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ]
    ]
}

register_structs! {
    #[allow(non_snake_case)]
    RegisterBlock {
        (0x00 => _reserved1),
        (0x04 => AUXENB: ReadWrite<u32, AUXENB::Register>),
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
        (0x68 => AUX_MU_BAUD: ReadWrite<u32>),
        (0x6c => _reserved5),
        (0xd8 => @END),
    }
}

type Registers = MMIODerefWrapper<RegisterBlock>;

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
}

impl console::Read for MiniUart {
    fn read_char(&self) -> char {
        let inner = self.inner.lock().unwrap();
        inner.read_byte() as char
    }
}

impl console::All for MiniUart {}
