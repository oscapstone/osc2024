use crate::mmio::regs::MmioReg;
use crate::mmio::regs::PmReg;
use crate::mmio::Mmio;

#[allow(dead_code)]
pub fn reset(tick: u32) {
    const PM_PASSWORD: u32 = 0x5A00_0000;
    Mmio::write_reg(MmioReg::Pm(PmReg::Rstc), PM_PASSWORD | 0x20);
    Mmio::write_reg(MmioReg::Pm(PmReg::Wdog), PM_PASSWORD | tick);
}
