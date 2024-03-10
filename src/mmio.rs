use core::ptr::read_volatile;
use core::ptr::write_volatile;

const MMIO_BASE: u32 = 0x3F000000;

pub struct MMIO;

impl MMIO {
    pub const AUX_ENABLE: u32 = MMIO_BASE + 0x00215004;
    pub const AUX_MU_IO: u32 = MMIO_BASE + 0x00215040;
    pub const AUX_MU_IER: u32 = MMIO_BASE + 0x00215044;
    pub const AUX_MU_LCR: u32 = MMIO_BASE + 0x0021504C;
    pub const AUX_MU_MCR: u32 = MMIO_BASE + 0x00215050;
    pub const AUX_MU_IIR: u32 = MMIO_BASE + 0x00215048;
    pub const AUX_MU_LSR: u32 = MMIO_BASE + 0x00215054;
    // pub const AUX_MU_MSR: u32 = MMIO_BASE + 0x00215058;
    // pub const AUX_MU_SCRATCH: u32 = MMIO_BASE + 0x0021505C;
    pub const AUX_MU_CNTL: u32 = MMIO_BASE + 0x00215060;
    // pub const AUX_MU_STAT: u32 = MMIO_BASE + 0x00215064;
    pub const AUX_MU_BAUD: u32 = MMIO_BASE + 0x00215068;

    // pub const GPFSEL0: u32 = MMIO_BASE + 0x00200000;
    pub const GPFSEL1: u32 = MMIO_BASE + 0x00200004;
    pub const GPPUD: u32 = MMIO_BASE + 0x00200094;
    pub const GPPUDCLK0: u32 = MMIO_BASE + 0x00200098;
    // pub const GPPUDCLK1: u32 = MMIO_BASE + 0x0020009C;

    pub unsafe fn write_reg(addr: u32, value: u32) {
        write_volatile(addr as *mut u32, value);
    }

    pub unsafe fn read_reg(addr: u32) -> u32 {
        read_volatile(addr as *const u32)
    }

    pub fn delay(count: u32) {
        for _ in 0..count {
            unsafe {
                core::arch::asm!("nop");
            }
        }
    }
}
