use core::ptr::read_volatile;
use core::ptr::write_volatile;

#[derive(Copy, Clone)]
pub enum Register {
    AUX_IRQ = 0x3F21_5000,
    AUX_ENABLES = 0x3F21_5004,
    AUX_MU_IO_REG = 0x3F21_5040,
    AUX_MU_IER_REG = 0x3F21_5044,
    AUX_MU_IIR_REG = 0x3F21_5048,
    AUX_MU_LCR_REG = 0x3F21_504C,
    AUX_MU_MCR_REG = 0x3F21_5050,
    AUX_MU_LSR_REG = 0x3F21_5054,
    AUX_MU_MSR_REG = 0x3F21_5058,
    AUX_MU_SCRATCH = 0x3F21_505C,
    AUX_MU_CNTL_REG = 0x3F21_5060,
    AUX_MU_STAT_REG = 0x3F21_5064,
    AUX_MU_BAUD_REG = 0x3F21_5068,
    GPFSEL0 = 0x3F20_0000,
    GPFSEL1 = 0x3F20_0004,
    GPFSEL2 = 0x3F20_0008,
    GPFSEL3 = 0x3F20_000C,
    GPFSEL4 = 0x3F20_0010,
    GPFSEL5 = 0x3F20_0014,
    GPPUD = 0x3F20_0094,
    GPPUDCLK0 = 0x3F20_0098,
    IRQs1 = 0x3F00_B210,
}

impl Register {
    pub fn addr(&self) -> u32 {
        *self as u32
    }
}

pub struct MMIO {}

impl MMIO {
    pub fn read(reg: Register) -> u32 {
        unsafe {
            read_volatile(reg.addr() as *const u32)
        }
    }

    pub fn write(reg: Register, data: u32) {
        unsafe {
            write_volatile(reg.addr() as *mut u32, data);
        }
    }
}
