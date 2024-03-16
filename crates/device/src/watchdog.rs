use small_std::sync::Mutex;
use tock_registers::{interfaces::Writeable, register_structs, registers::ReadWrite};

use crate::{common::MMIODerefWrapper, driver::DeviceDriver};

const PM_PASSWORD: u32 = 0x5a00_0000;

register_structs! {
    #[allow(non_snake_case)]
    RegisterBlock {
        (0x00 => _reserved1),
        (0x1c => PM_RSTC: ReadWrite<u32>),
        (0x20 => _reserved2),
        (0x24 => PM_WDOG: ReadWrite<u32>),
        (0x28 => @END),
    }
}

type Registers = MMIODerefWrapper<RegisterBlock>;

struct WatchdogInner {
    registers: Registers,
}

pub struct Watchdog {
    inner: Mutex<WatchdogInner>,
}

impl WatchdogInner {
    /// SAFETY: The user msut ensure to provide a correct MMIO start address
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: Registers::new(mmio_start_addr),
        }
    }

    fn reset(&self, tick: u32) {
        self.registers.PM_RSTC.set(PM_PASSWORD | 0x20);
        self.registers.PM_WDOG.set(PM_PASSWORD | tick);
    }

    fn cancel_reset(&self) {
        self.registers.PM_RSTC.set(PM_PASSWORD);
        self.registers.PM_WDOG.set(PM_PASSWORD);
    }
}

impl Watchdog {
    const COMPATIBLE: &'static str = "Watchdog";

    /// SAFETY: The user msut ensure to provide a correct MMIO start address
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: Mutex::new(WatchdogInner::new(mmio_start_addr)),
        }
    }

    pub fn reset(&self, tick: u32) {
        let inner = self.inner.lock().unwrap();
        inner.reset(tick);
    }

    pub fn cancel_reset(&self) {
        let inner = self.inner.lock().unwrap();
        inner.cancel_reset();
    }
}

impl DeviceDriver for Watchdog {
    fn compatible(&self) -> &'static str {
        Self::COMPATIBLE
    }
}
