use mmio::MMIO;

pub unsafe fn uart_init() {
    // Enable mini UART
    MMIO::write_reg(MMIO::AUX_ENABLE, MMIO::read_reg(MMIO::AUX_ENABLE) | 1);

    // Disable transmitter and receiver during configuration
    MMIO::write_reg(MMIO::AUX_MU_CNTL, 0);

    // Configure UART
    MMIO::write_reg(MMIO::AUX_MU_LCR, 3); // Set the data size to 8 bit
    MMIO::write_reg(MMIO::AUX_MU_MCR, 0); // No auto flow control
    MMIO::write_reg(MMIO::AUX_MU_BAUD, 270); // Set baud rate for 115200
    MMIO::write_reg(MMIO::AUX_MU_IIR, 6); // No FIFO

    // Map UART1 to GPIO pins
    let mut reg = MMIO::read_reg(MMIO::GPFSEL1);
    reg &= !((7 << 12) | (7 << 15)); // Clear existing settings for GPIO 14, 15
    reg |= (2 << 12) | (2 << 15); // Set to alt5 for mini UART
    MMIO::write_reg(MMIO::GPFSEL1, reg);

    // Disable pull-up/down for pins 14 and 15
    MMIO::write_reg(MMIO::GPPUD, 0);
    MMIO::delay(150);
    MMIO::write_reg(MMIO::GPPUDCLK0, (1 << 14) | (1 << 15));
    MMIO::delay(150);
    MMIO::write_reg(MMIO::GPPUDCLK0, 0);

    // Enable the transmitter and receiver
    MMIO::write_reg(MMIO::AUX_MU_CNTL, 3);
    MMIO::write_reg(MMIO::AUX_MU_IER, 0); // Disable interrupt
}

pub unsafe fn uart_send(c: u8) {
    // Wait until we can send
    while MMIO::read_reg(MMIO::AUX_MU_LSR) & 0x20 == 0 {
        core::arch::asm!("nop");
    }

    // Write the character to the buffer
    MMIO::write_reg(MMIO::AUX_MU_IO, c as u32);

    // If the character is newline, also send carriage return
    if c == b'\n' {
        uart_send(b'\r');
    }
}

pub unsafe fn uart_recv() -> u8 {
    // Wait until we can receive
    while MMIO::read_reg(MMIO::AUX_MU_LSR) & 0x01 == 0 {
        core::arch::asm!("nop");
    }

    // Read the character from the buffer
    let c: u8 = MMIO::read_reg(MMIO::AUX_MU_IO) as u8;
    match c {
        b'\r' => b'\n',
        _ => c,
    }
}

#[allow(dead_code)]
pub unsafe fn uart_read(buf: &mut [u8]) {
    for i in buf.iter_mut() {
        *i = uart_recv();
    }
}

pub unsafe fn uart_write(buf: &[u8]) {
    for &c in buf {
        uart_send(c);
    }
}

pub unsafe fn uart_puts(buf: &[u8]) {
    for &c in buf {
        if c == 0 {
            break;
        }
        uart_send(c);
    }
    uart_send(b'\n');
}
