use super::interrupt_controller::InterruptNumber;

use super::common::MMIODerefWrapper;
use super::device_driver::DeviceDriver;
use cpu::cpu::spin_for_cycle;
use library::sync::mutex::Mutex;
use tock_registers::{
    interfaces::{ReadWriteable, Writeable},
    register_bitfields, register_structs,
    registers::ReadWrite,
};

register_bitfields! [
    u32,
    // GPIO function select 1
    GPFSEL1 [
        // Pin 15
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
        // Pin 14
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
    // GPIO pull down / up register
    GPPUD [
        PUD OFFSET(0) NUMBITS(2) [
            Off = 0b00,
            PullDown = 0b01,
            PullUp = 0b10,
            Reserved = 0b11,
        ]
    ],
    // GPIO pull down / up register 0
    GPPUDCLK0 [
        PUDCLK15 OFFSET(15) NUMBITS(1) [
            NoEffect = 0,
            AssertClock = 1,
        ],
        PUDCLK14 OFFSET(14) NUMBITS(1) [
            NoEffect = 0,
            AssertClock = 1,
        ]
    ]
];

register_structs! {
    Registers {
        // GPFSEL0
        (0x00 => _reserved1),
        (0x04 => gpfsel1: ReadWrite<u32, GPFSEL1::Register>),
        // GPFSEL2 ...
        (0x08 => _reserved2),
        (0x94 => gppud: ReadWrite<u32, GPPUD::Register>),
        (0x98 => gppudclk0: ReadWrite<u32, GPPUDCLK0::Register>),
        (0x9c => _reserved3),
        (0xb4 => @END),
    }
}

pub struct GPIOInner {
    registers: MMIODerefWrapper<Registers>,
}

pub struct GPIO {
    inner: Mutex<GPIOInner>,
}

impl GPIOInner {
    /**
     * # Safety
     *
     * - The user must ensure to provide a correct MMIO start address.
     */
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: MMIODerefWrapper::new(mmio_start_addr),
        }
    }

    fn disable_pud_14_15(&mut self) {
        // disable pin pull-up/down
        // for pin 14, 15
        self.registers.gppud.write(GPPUD::PUD::Off);
        spin_for_cycle(150);
        self.registers
            .gppudclk0
            .write(GPPUDCLK0::PUDCLK15::AssertClock + GPPUDCLK0::PUDCLK14::AssertClock);
        spin_for_cycle(150);
        self.registers.gppud.write(GPPUD::PUD::Off);
        self.registers.gppudclk0.set(0);
    }

    /**
     * Setup GPIO for mini UART
     */
    pub fn setup_for_mini_uart(&mut self) {
        self.registers
            .gpfsel1
            .modify(GPFSEL1::FSEL15::AltFunc5 + GPFSEL1::FSEL14::AltFunc5);
        self.disable_pud_14_15();
    }

    /**
     * Setup GPIO for PL011 UART
     */
    pub fn setup_for_pl011_uart(&mut self) {
        self.registers
            .gpfsel1
            .modify(GPFSEL1::FSEL15::AltFunc0 + GPFSEL1::FSEL14::AltFunc0);
        self.disable_pud_14_15();
    }
}

impl GPIO {
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: Mutex::new(GPIOInner::new(mmio_start_addr)),
        }
    }

    pub fn setup_for_mini_uart(&self) {
        let mut inner = self.inner.lock().unwrap();
        inner.setup_for_mini_uart();
    }
}

impl DeviceDriver for GPIO {
    type InterruptNumberType = InterruptNumber;
}
