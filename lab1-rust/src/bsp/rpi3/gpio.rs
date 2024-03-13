use super::physical_memory::map::mmio::GPIO_START;
use crate::bsp::common::MMIODerefWrapper;
use aarch64_cpu::asm;
use tock_registers::{
    interfaces::{ReadWriteable, Writeable},
    register_bitfields, register_structs,
    registers::ReadWrite,
};

register_bitfields! {
    u32,

    /// GPIO Function Select 1
    GPFSEL1 [
        /// Pin 15
        FSEL15 OFFSET(15) NUMBITS(3) [
            Input = 0b000,
            Output = 0b001,
            AltFunc0 = 0b100,  // PL011 UART RX
            AltFunc5 = 0b010   // mini UART RX
        ],
        /// Pin 14
        FSEL14 OFFSET(12) NUMBITS(3) [
            Input = 0b000,
            Output = 0b001,
            AltFunc0 = 0b100,  // PL011 UART TX
            AltFunc5 = 0b010   // mini UART TX
        ]
    ],

    /// GPIO Pull-up/down Register
    GPPUD [
        PUD OFFSET(0) NUMBITS(2) [
            Off = 0b00,
            PullDown = 0b01,
            PulREGISTERSlUp = 0b10
        ]
    ],

    /// GPIO Pull-up/down Clock Register 0
    GPPUDCLK0 [
        /// Pin 15
        PUDCLK15 OFFSET(15) NUMBITS(1) [
            NoEffect = 0,
            AssertClock = 1
        ],

        /// Pin 14
        PUDCLK14 OFFSET(14) NUMBITS(1) [
            NoEffect = 0,
            AssertClock = 1
        ]
    ],
}

register_structs! {
    #[allow(non_snake_case)]
    RegisterBlock {
        (0x00 => _reserved1),
        (0x04 => GPFSEL1: ReadWrite<u32, GPFSEL1::Register>),
        (0x08 => _reserved2),
        (0x94 => GPPUD: ReadWrite<u32, GPPUD::Register>),
        (0x98 => GPPUDCLK0: ReadWrite<u32, GPPUDCLK0::Register>),
        (0x9C => @END),
    }
}

/// Abstraction for the associated MMIO registers.
type Registers = MMIODerefWrapper<RegisterBlock>;

pub struct GPIO {
    registers: Registers,
}

impl GPIO {
    /// Create an instance.
    ///
    /// # Safety
    ///
    /// - The user must ensure to provide a correct MMIO start address.
    pub const unsafe fn new() -> Self {
        GPIO {
            registers: Registers::new(GPIO_START),
        }
    }
    /// Initialize the GPIO with the MMIO start address.
    pub fn init(&self) {
        self.map_mini_uart();
    }

    /// Map Mini UART as standard output.
    fn map_mini_uart(&self) {
        self.registers
            .GPFSEL1
            .modify(GPFSEL1::FSEL15::AltFunc5 + GPFSEL1::FSEL14::AltFunc5);

        self.disable_pud_14_15();
    }

    /// Disable pull-up/down on pins 14 and 15.
    fn disable_pud_14_15(&self) {
        let delay: usize = 150; // 延遲

        self.registers.GPPUD.write(GPPUD::PUD::Off);
        for _ in 0..delay {
            asm::nop();
        }

        self.registers
            .GPPUDCLK0
            .write(GPPUDCLK0::PUDCLK15::AssertClock + GPPUDCLK0::PUDCLK14::AssertClock);
        for _ in 0..delay {
            asm::nop();
        }

        self.registers.GPPUD.write(GPPUD::PUD::Off);
        self.registers.GPPUDCLK0.set(0);
    }
}
