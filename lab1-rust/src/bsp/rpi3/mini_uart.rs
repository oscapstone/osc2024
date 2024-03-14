use super::physical_memory::map::mmio::MINI_UART_START;
use crate::bsp::common::MMIODerefWrapper;
use tock_registers::{
    interfaces::{Readable, Writeable},
    register_bitfields, register_structs,
    registers::{ReadOnly, ReadWrite},
};

use aarch64_cpu::asm;

register_bitfields! {
    u32,
    AUX_ENABLE [
        MINI_UART_ENABLE OFFSET(0) NUMBITS(1) [
            Disabled = 0,
            Enabled = 1
        ]
    ],
    AUX_MU_IER [
        RX_INT_ENABLE OFFSET(1) NUMBITS(1) [
            Disabled = 0,
            Enabled = 1
        ],
        TX_INT_ENABLE OFFSET(0) NUMBITS(1) [
            Disabled = 0,
            Enabled = 1
        ]
    ],
    AUX_MU_IIR [
        INT_PENDING OFFSET(0) NUMBITS(1) [
            NoInterrupt = 0,
            Interrupt = 1
        ]
    ],
    AUX_MU_LCR [
        WLEN OFFSET(0) NUMBITS(2) [
            SevenBit = 0b00,
            EightBit = 0b11
        ],
    ],
    AUX_MU_MCR [
        RTS OFFSET(1) NUMBITS(1) [
            Inactive = 0,
            Active = 1
        ]
    ],
    AUX_MU_LSR [
        RX_READY OFFSET(0) NUMBITS(1) [
            Empty = 0,
            Data = 1
        ],
        TX_EMPTY OFFSET(5) NUMBITS(1) [
            Full = 0,
            Empty = 1
        ]
    ],
    AUX_MU_CNTL [
        RX_ENABLE OFFSET(1) NUMBITS(1) [
            Disabled = 0,
            Enabled = 1
        ],
        TX_ENABLE OFFSET(0) NUMBITS(1) [
            Disabled = 0,
            Enabled = 1
        ]
    ],
    AUX_MU_BAUD [
        BAUDRATE OFFSET(0) NUMBITS(16) [],
    ],
}

register_structs! {
    #[allow(non_snake_case)]
    pub RegisterBlock {
        (0x00 => _reserved1),
        (0x04 => AUX_ENABLE: ReadWrite<u32, AUX_ENABLE::Register>),
        (0x08 => _reserved2),
        (0x40 => AUX_MU_IO: ReadWrite<u32>),
        (0x44 => AUX_MU_IER: ReadWrite<u32, AUX_MU_IER::Register>),
        (0x48 => AUX_MU_IIR: ReadWrite<u32, AUX_MU_IIR::Register>),
        (0x4C => AUX_MU_LCR: ReadWrite<u32, AUX_MU_LCR::Register>),
        (0x50 => AUX_MU_MCR: ReadWrite<u32, AUX_MU_MCR::Register>),
        (0x54 => AUX_MU_LSR: ReadOnly<u32, AUX_MU_LSR::Register>),
        (0x58 => AUX_MU_MSR: ReadOnly<u32>),
        (0x5C => AUX_MU_SCRATCH: ReadWrite<u32>),
        (0x60 => AUX_MU_CNTL: ReadWrite<u32, AUX_MU_CNTL::Register>),
        (0x64 => AUX_MU_STAT: ReadOnly<u32>),
        (0x68 => AUX_MU_BAUD: ReadWrite<u32, AUX_MU_BAUD::Register>),
        (0x6C => @END),
    }
}

/// Abstraction for the associated MMIO registers.
type Registers = MMIODerefWrapper<RegisterBlock>;

pub struct MiniUart {
    registers: Registers,
}

impl MiniUart {
    pub const unsafe fn new() -> Self {
        Self {
            registers: Registers::new(MINI_UART_START),
        }
    }

    pub fn setup_mini_uart(&self) {
        // Enable the mini UART.
        self.registers
            .AUX_ENABLE
            .write(AUX_ENABLE::MINI_UART_ENABLE::Enabled);
        // Disable TX, RX during configuration
        self.registers
            .AUX_MU_CNTL
            .write(AUX_MU_CNTL::RX_ENABLE::Disabled + AUX_MU_CNTL::TX_ENABLE::Disabled);
        // Disable interrupt
        self.registers
            .AUX_MU_IER
            .write(AUX_MU_IER::RX_INT_ENABLE::Disabled + AUX_MU_IER::TX_INT_ENABLE::Disabled);
        // Set the data size to 8 bit
        self.registers.AUX_MU_LCR.write(AUX_MU_LCR::WLEN::EightBit);
        // Don't need auto flow control
        self.registers.AUX_MU_MCR.write(AUX_MU_MCR::RTS::Inactive);
        // Set baud rate to 115200
        self.registers
            .AUX_MU_BAUD
            .write(AUX_MU_BAUD::BAUDRATE.val(270));
        // No FIFO
        self.registers
            .AUX_MU_IIR
            .write(AUX_MU_IIR::INT_PENDING::NoInterrupt);
    }

    pub fn send(&self, c: u8) {
        while !self.registers.AUX_MU_LSR.is_set(AUX_MU_LSR::TX_EMPTY) {
            // asm::nop();
        } // when TX is not empty, wait
        self.registers.AUX_MU_IO.set(c as u32);
    }

    pub fn receive(&self) -> u8 {
        while !self.registers.AUX_MU_LSR.is_set(AUX_MU_LSR::RX_READY) {
            // asm::nop();
        } // when RX is not ready, wait
        self.registers.AUX_MU_IO.get() as u8
    }
}
