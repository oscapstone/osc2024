use small_std::sync::Mutex;
use tock_registers::{
    interfaces::{ReadWriteable, Writeable},
    register_bitfields, register_structs,
    registers::ReadWrite,
};

use crate::{common::MMIODerefWrapper, driver::DeviceDriver};

// GPIO registers.
//
// Descriptions taken from
// - https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf
// - https://datasheets.raspberrypi.org/bcm2711/bcm2711-peripherals.pdf
register_bitfields! [
    u32,
    /// GPIO Function Select 1
    GPFSEL1 [
        /// Pin 15
        FSEL15 OFFSET(15) NUMBITS(3) [
            Input = 0b000,
            Output = 0b001,
            AltFunc0 = 0b100,
            AltFunc1 = 0b101,
            AltFunc2 = 0b110,
            AltFunc3 = 0b111,
            AltFunc4 = 0b011,
            AltFunc5 = 0b010,
        ],
        /// Pin 14
        FSEL14 OFFSET(12) NUMBITS(3) [
            Input = 0b000,
            Output = 0b001,
            AltFunc0 = 0b100,
            AltFunc1 = 0b101,
            AltFunc2 = 0b110,
            AltFunc3 = 0b111,
            AltFunc4 = 0b011,
            AltFunc5 = 0b010,
        ]
    ],
    /// GPIO Pull-up/down Register
    GPPUD [
        /// Controls the actuation of the internal pull-up/down control line to ALL the GPIO pins.
        PUD OFFSET(0) NUMBITS(2) [
            Off = 0b00,
            PullDown = 0b01,
            PullUp = 0b10,
            Reserved = 0b11,
        ]
    ],
    /// GPIO Pull-up/down Clock Register 0
    GPPUDCLK0 [
        /// Pin 15
        PUDCLK15 OFFSET(15) NUMBITS(1) [
            NoEffect = 0,
            AssertClock = 1,
        ],
        /// Pin 14
        PUDCLK14 OFFSET(14) NUMBITS(1) [
            NoEffect = 0,
            AssertClock = 1,
        ]
    ]
];

register_structs! {
    #[allow(non_snake_case)]
    RegisterBlock {
        (0x00 => _reserved1),
        (0x04 => GPFSEL1: ReadWrite<u32, GPFSEL1::Register>),
        (0x08 => _reserved2),
        (0x94 => GPPUD: ReadWrite<u32, GPPUD::Register>),
        (0x98 => GPPUDCLK0: ReadWrite<u32, GPPUDCLK0::Register>),
        (0x9c => _reserved3),
        (0xb4 => @END),
    }
}

type Registers = MMIODerefWrapper<RegisterBlock>;

struct GPIOInner {
    registers: Registers,
}

pub struct GPIO {
    inner: Mutex<GPIOInner>,
}

impl GPIOInner {
    /// SAFETY: The user msut ensure to provide a correct MMIO start address
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: Registers::new(mmio_start_addr),
        }
    }

    /// Disable pull-up/down on pins 14 and 15.
    fn disable_pud_14_15(&mut self) {
        const DELAY: usize = 2000;

        self.registers.GPPUD.write(GPPUD::PUD::Off);
        cpu::spin_for_cycles(DELAY);

        self.registers
            .GPPUDCLK0
            .write(GPPUDCLK0::PUDCLK15::AssertClock + GPPUDCLK0::PUDCLK14::AssertClock);
        cpu::spin_for_cycles(DELAY);

        self.registers.GPPUD.write(GPPUD::PUD::Off);
        self.registers.GPPUDCLK0.set(0);
    }

    /// Map Mini UART as standard output.
    pub fn map_mini_uart(&mut self) {
        self.registers
            .GPFSEL1
            .modify(GPFSEL1::FSEL15::AltFunc5 + GPFSEL1::FSEL14::AltFunc5);
        self.disable_pud_14_15();
    }
}

impl GPIO {
    pub const COMPATIBLE: &'static str = "GPIO";

    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: Mutex::new(GPIOInner::new(mmio_start_addr)),
        }
    }

    /// Concurrency safe version of `GPIOInner::map_mini_uart`
    pub fn map_mini_uart(&self) {
        let mut inner = self.inner.lock().unwrap();
        inner.map_mini_uart();
    }
}

impl DeviceDriver for GPIO {
    fn compatible(&self) -> &'static str {
        Self::COMPATIBLE
    }
}
