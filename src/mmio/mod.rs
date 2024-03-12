use crate::mmio::regs::MmioReg;
use crate::mmio::regs::PmReg;

pub mod mailbox;
pub mod regs;

pub struct MMIO;

impl MMIO {
    pub fn write_reg(reg: MmioReg, value: u32) {
        let addr = reg.addr();
        unsafe { core::ptr::write_volatile(addr as *mut u32, value) }
    }

    pub fn read_reg(reg: MmioReg) -> u32 {
        let addr = reg.addr();
        unsafe { core::ptr::read_volatile(addr as *const u32) }
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
        MMIO::write_reg(MmioReg::Pm(PmReg::Rstc), PM_PASSWORD | 0x20);
        MMIO::write_reg(MmioReg::Pm(PmReg::Wdog), PM_PASSWORD | 1 << 14);
    }
}
