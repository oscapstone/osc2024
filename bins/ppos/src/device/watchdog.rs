use library::sync::mutex::Mutex;
use tock_registers::{interfaces::Writeable, register_structs, registers::ReadWrite};

use super::{
    common::MMIODerefWrapper, device_driver::DeviceDriver, interrupt_controller::InterruptNumber,
};

const PM_PASSWORD: u32 = 0x5a000000;

register_structs! {
    Registers {
        (0x00 => _reserved1),
        (0x1c => rstc: ReadWrite<u32>),
        (0x20 => _reserved2),
        (0x24 => wdog: ReadWrite<u32>),
        (0x28 => @END),
    }
}

struct WatchdogInner {
    registers: MMIODerefWrapper<Registers>,
}

impl WatchdogInner {
    /**
     * # Safety
     *
     * - The user must ensure to provide a correct MMIO start address.
     */
    const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: MMIODerefWrapper::new(mmio_start_addr),
        }
    }

    fn reset(&self, tick: u32) {
        self.registers.rstc.set(PM_PASSWORD | 0x20);
        self.registers.wdog.set(PM_PASSWORD | tick);
    }

    fn cancel_reset(&self) {
        self.registers.rstc.set(PM_PASSWORD);
        self.registers.wdog.set(PM_PASSWORD);
    }
}

pub struct Watchdog {
    inner: Mutex<WatchdogInner>,
}

impl DeviceDriver for Watchdog {
    type InterruptNumberType = InterruptNumber;
}

impl Watchdog {
    /**
     * # Safety
     *
     * - The user must ensure to provide a correct MMIO start address.
     */
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
