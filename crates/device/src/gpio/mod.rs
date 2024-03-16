mod registers;

use registers::{Registers, GPFSEL1, GPPUD, GPPUDCLK0};
use small_std::sync::Mutex;
use tock_registers::interfaces::{ReadWriteable, Writeable};

use crate::driver::DeviceDriver;

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
