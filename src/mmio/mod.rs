use crate::mmio::regs::MmioReg;
use crate::mmio::regs::PmReg;

pub mod regs;

pub struct MMIO;

impl MMIO {
    pub unsafe fn write_reg(reg: MmioReg, value: u32) {
        let addr = reg.addr();
        core::ptr::write_volatile(addr as *mut u32, value);
    }

    pub unsafe fn read_reg(reg: MmioReg) -> u32 {
        let addr = reg.addr();
        core::ptr::read_volatile(addr as *const u32)
    }

    pub fn delay(count: u32) {
        for _ in 0..count {
            unsafe {
                core::arch::asm!("nop");
            }
        }
    }

    pub fn reboot() {
        const PM_PASSWORD: u32 = 0x5A00_0000;
        unsafe {
            MMIO::write_reg(MmioReg::Pm(PmReg::Rstc), PM_PASSWORD | 0x20);
            MMIO::write_reg(MmioReg::Pm(PmReg::Wdog), PM_PASSWORD | 1 << 9);
        }
    }
}
