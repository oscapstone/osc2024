const MMIO_BASE: u32 = 0x3F000000;
const AUX_ENABLE: *mut u32 = (MMIO_BASE + 0x00215004) as *mut u32;
const AUX_MU_IO: *mut u32 = (MMIO_BASE + 0x00215040) as *mut u32;
const AUX_MU_IER: *mut u32 = (MMIO_BASE + 0x00215044) as *mut u32;
const AUX_MU_IIR: *mut u32 = (MMIO_BASE + 0x00215048) as *mut u32;
const AUX_MU_LCR: *mut u32 = (MMIO_BASE + 0x0021504C) as *mut u32;
const AUX_MU_MCR: *mut u32 = (MMIO_BASE + 0x00215050) as *mut u32;
const AUX_MU_LSR: *mut u32 = (MMIO_BASE + 0x00215054) as *mut u32;
const AUX_MU_MSR: *mut u32 = (MMIO_BASE + 0x00215058) as *mut u32;
const AUX_MU_SCRATCH: *mut u32 = (MMIO_BASE + 0x0021505C) as *mut u32;
const AUX_MU_CNTL: *mut u32 = (MMIO_BASE + 0x00215060) as *mut u32;
const AUX_MU_STAT: *mut u32 = (MMIO_BASE + 0x00215064) as *mut u32;
const AUX_MU_BAUD: *mut u32 = (MMIO_BASE + 0x00215068) as *mut u32;

const GPFSEL0: *mut u32 = (MMIO_BASE + 0x00200000) as *mut u32;
const GPFSEL1: *mut u32 = (MMIO_BASE + 0x00200004) as *mut u32;
const GPPUD: *mut u32 = (MMIO_BASE + 0x00200094) as *mut u32;
const GPPUDCLK0: *mut u32 = (MMIO_BASE + 0x00200098) as *mut u32;

use core::arch::asm;

fn delay(count: u32) {
    for _ in 0..count {
        unsafe {
            asm!("nop");
        }
    }
}

pub unsafe fn uart_init() {
    // Enable mini UART
    AUX_ENABLE.write_volatile(AUX_ENABLE.read_volatile() | 1);

    // Disable transmitter and receiver during configuration
    AUX_MU_CNTL.write_volatile(0);

    // Configure UART

    AUX_MU_LCR.write_volatile(3); // Set the data size to 8 bit
    AUX_MU_MCR.write_volatile(0); // No auto flow control
    AUX_MU_BAUD.write_volatile(270); // Set baud rate for 115200
    AUX_MU_IIR.write_volatile(6); // No FIFO

    // Map UART1 to GPIO pins
    let mut reg = GPFSEL1.read_volatile();
    reg &= !((7 << 12) | (7 << 15)); // Clear existing settings for GPIO 14, 15
    reg |= (2 << 12) | (2 << 15); // Set to alt5 for mini UART
    GPFSEL1.write_volatile(reg);

    // Disable pull-up/down for pins 14 and 15
    GPPUD.write_volatile(0);
    delay(150);
    GPPUDCLK0.write_volatile((1 << 14) | (1 << 15));
    delay(150);
    GPPUDCLK0.write_volatile(0);

    // Enable the transmitter and receiver
    AUX_MU_CNTL.write_volatile(3);
    AUX_MU_IER.write_volatile(0); // Disable interrupt
}

pub unsafe fn uart_send(c: u8) {
    // Wait until we can send
    while AUX_MU_LSR.read_volatile() & 0x20 == 0 {
        asm!("nop");
    }

    // Write the character to the buffer
    AUX_MU_IO.write_volatile(c as u32);

    // If the character is newline, also send carriage return
    if c == '\n' as u8 {
        uart_send('\r' as u8);
    }
}

pub unsafe fn uart_recv() -> u8 {
    // Wait until we can receive
    while AUX_MU_LSR.read_volatile() & 0x01 == 0 {
        asm!("nop");
    }

    // Read the character from the buffer
    let c: u8 = AUX_MU_IO.read_volatile() as u8;
    match c {
        b'\r' => b'\n',
        _ => c,
    }
}

pub unsafe fn uart_read(buf: &mut [u8]) {
    for i in 0..buf.len() {
        buf[i] = uart_recv();
    }
}

pub unsafe fn uart_write(buf: &[u8]) {
    for &c in buf {
        uart_send(c);
    }
}
