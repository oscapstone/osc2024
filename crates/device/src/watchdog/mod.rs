mod registers;

use registers::Registers;
use small_std::sync::Mutex;
use tock_registers::interfaces::Writeable;

use crate::driver::DeviceDriver;

const PM_PASSWORD: u32 = 0x5a00_0000;

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
