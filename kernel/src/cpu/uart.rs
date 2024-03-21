use crate::cpu::registers::MMIO;
use crate::cpu::registers::Register;
use core::arch::asm;

pub unsafe fn initialize() {
    // GPIO
    MMIO::write(Register::GPFSEL1, (2 << 12) | (2 << 15));
    MMIO::write(Register::GPPUD, 0);
    for _ in 0..300 {
        asm!("nop");
    }
    MMIO::write(Register::GPPUDCLK0, 3 << 14);
    for _ in 0..300 {
        asm!("nop");
    }
    MMIO::write(Register::GPPUD, 0);
    MMIO::write(Register::GPPUDCLK0, 0);

    // UART
    MMIO::write(Register::AUX_ENABLES, 1);
    MMIO::write(Register::AUX_MU_CNTL_REG, 0);
    MMIO::write(Register::AUX_MU_IER_REG, 0);
    MMIO::write(Register::AUX_MU_LCR_REG, 3);
    MMIO::write(Register::AUX_MU_MCR_REG, 0);
    MMIO::write(Register::AUX_MU_BAUD_REG, 270);
    MMIO::write(Register::AUX_MU_IIR_REG, 6);
    MMIO::write(Register::AUX_MU_CNTL_REG, 3);
}

pub unsafe fn send(c: u8) {
    while (MMIO::read(Register::AUX_MU_LSR_REG) & (1 << 5)) == 0 {}
    MMIO::write(Register::AUX_MU_IO_REG, c as u32);
}

pub unsafe fn recv() -> u8 {
    while (MMIO::read(Register::AUX_MU_LSR_REG) & 0x01) == 0 {}
    MMIO::read(Register::AUX_MU_IO_REG) as u8
}