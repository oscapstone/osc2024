use tock_registers::{register_bitfields, register_structs, registers::ReadWrite};

use crate::common::MMIODerefWrapper;

// GPIO registers.
//
// Descriptions taken from
// - https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf
// - https://datasheets.raspberrypi.org/bcm2711/bcm2711-peripherals.pdf
register_bitfields! [
    u32,

    /// GPIO Function Select 1
    pub GPFSEL1 [
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
    pub GPPUD [
        /// Controls the actuation of the internal pull-up/down control line to ALL the GPIO pins.
        PUD OFFSET(0) NUMBITS(2) [
            Off = 0b00,
            PullDown = 0b01,
            PullUp = 0b10,
            Reserved = 0b11,
        ]
    ],
    /// GPIO Pull-up/down Clock Register 0
    pub GPPUDCLK0 [
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
    pub RegisterBlock {
        (0x00 => _reserved1),
        (0x04 => pub GPFSEL1: ReadWrite<u32, GPFSEL1::Register>),
        (0x08 => _reserved2),
        (0x94 => pub GPPUD: ReadWrite<u32, GPPUD::Register>),
        (0x98 => pub GPPUDCLK0: ReadWrite<u32, GPPUDCLK0::Register>),
        (0x9c => _reserved3),
        (0xb4 => @END),
    }
}

pub type Registers = MMIODerefWrapper<RegisterBlock>;
