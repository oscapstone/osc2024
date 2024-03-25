use mmio::regs::AuxReg::*;
use mmio::regs::GpioReg::*;
use mmio::regs::MmioReg::{Aux, Gpio};
use mmio::MMIO;

pub fn uart_init() {
    // Enable mini UART
    MMIO::write_reg(Aux(Enable), 1);

    // Disable transmitter and receiver during configuration
    MMIO::write_reg(Aux(MuCntl), 0);

    // Configure UART
    MMIO::write_reg(Aux(MuIer), 0);
    MMIO::write_reg(Aux(MuLcr), 3); // Set the data size to 8 bit
    MMIO::write_reg(Aux(MuMcr), 0); // No auto flow control
    MMIO::write_reg(Aux(MuBaud), 270); // Set baud rate for 115200
    MMIO::write_reg(Aux(MuIir), 6); // No FIFOs (interrupt when receiver holds at least 1 byte

    // Map UART1 to GPIO pins
    let mut reg = MMIO::read_reg(Gpio(Gpfsel1));
    reg &= !((7 << 12) | (7 << 15)); // Clear existing settings for GPIO 14, 15
    reg |= (2 << 12) | (2 << 15); // Set to alt5 for mini UART
    MMIO::write_reg(Gpio(Gpfsel1), reg);

    // Disable pull-up/down for pins 14 and 15
    MMIO::write_reg(Gpio(Gppud), 0);
    MMIO::delay(150);
    MMIO::write_reg(Gpio(GppudClk0), (1 << 14) | (1 << 15));
    MMIO::delay(150);
    MMIO::write_reg(Gpio(GppudClk0), 0);

    // Enable the transmitter and receiver
    MMIO::write_reg(Aux(MuCntl), 3);
}

pub fn send(c: u8) {
    // Wait until we can send
    while MMIO::read_reg(Aux(MuLsr)) & 0x20 == 0 {
        MMIO::delay(1);
    }

    // Write the character to the buffer
    MMIO::write_reg(Aux(MuIo), c as u32);
}

pub fn recv() -> u8 {
    // Wait until we can receive
    let mut c = recv_nb();
    while c.is_none() {
        c = recv_nb();
    }
    c.unwrap()
}

pub fn recv_nb() -> Option<u8> {
    // Check if something to read
    if MMIO::read_reg(Aux(MuLsr)) & 0x01 == 0 {
        return None;
    }
    // Read the character from the buffer
    Some((MMIO::read_reg(Aux(MuIo)) & 0xFF) as u8)
}
