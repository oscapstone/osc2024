use crate::cpu::registers::Register;
use crate::cpu::registers::MMIO;
use crate::os::stdio::println;
use crate::{println, print};
use core::arch::asm;

const BUF_SIZE: usize = 1024;

static mut SEND_BUFFER: [u8; BUF_SIZE] = [0; BUF_SIZE];
static mut RECV_BUFFER: [u8; BUF_SIZE] = [0; BUF_SIZE];
static mut SEND_IDX: usize = 0;
static mut RECV_START_IDX: usize = 0;
static mut RECV_END_IDX: usize = 0;

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

    MMIO::write(Register::AUX_MU_IER_REG, 0x3);
    MMIO::write(Register::IRQs1, 1 << 29); // Enable mini UART interrupt
    SEND_IDX = 0;
    RECV_START_IDX = 0;
    RECV_END_IDX = 0;
}

pub unsafe fn send(c: u8) {
    while (MMIO::read(Register::AUX_MU_LSR_REG) & (1 << 5)) == 0 {}
    MMIO::write(Register::AUX_MU_IO_REG, c as u32);
}

pub unsafe fn recv() -> u8 {
    while (MMIO::read(Register::AUX_MU_LSR_REG) & 0x01) == 0 {}
    MMIO::read(Register::AUX_MU_IO_REG) as u8
}

pub unsafe fn non_blocking_recv() -> Option<u8> {
    if (MMIO::read(Register::AUX_MU_LSR_REG) & 0x01) == 0 {
        None
    } else {
        Some(MMIO::read(Register::AUX_MU_IO_REG) as u8)
    }
}

pub unsafe fn send_async(c: u8) {
    // asm!("msr DAIFSet, 0xf");
    SEND_BUFFER[SEND_IDX] = c;
    SEND_IDX += 1;
    while SEND_IDX == BUF_SIZE {}
    MMIO::write(Register::AUX_MU_IER_REG, 0b11);
    // println!("Send size: {}", SEND_IDX);
    // asm!("msr DAIFClr, 0xf");
}

pub unsafe fn recv_async() -> Option<u8> {
    // println!("RECV_START_IDX: {}", RECV_START_IDX);
    // println!("RECV_END_IDX: {}", RECV_END_IDX);
    if RECV_START_IDX == RECV_END_IDX {
        // println!("No data");
        None
    } else {
        let c = RECV_BUFFER[RECV_START_IDX];

        RECV_START_IDX += 1;
        RECV_START_IDX %= BUF_SIZE;
        // println!("RECV_START_IDX: {}", RECV_START_IDX);
        // println!("RECV_END_IDX: {}", RECV_END_IDX);
        Some(c)
    }
    
}

pub unsafe fn irq_handler() {
    // if SEND_IDX > 0 {
    //     println!("Send size: {}", SEND_IDX);
    // }

    for i in 0..SEND_IDX {
        send(SEND_BUFFER[i]);
    }
    SEND_IDX = 0;
    
    loop {
        match non_blocking_recv() {
            Some(c) => {
                RECV_BUFFER[RECV_END_IDX] = c;
                RECV_END_IDX += 1;
                RECV_END_IDX %= BUF_SIZE;
            }
            None => break,
        }
    }
    MMIO::write(Register::AUX_MU_IER_REG, 0b01);
    // println!("RECV: {}", RECV_END_IDX);
}
