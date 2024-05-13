use crate::mmio::regs::AuxReg::*;
use crate::mmio::regs::GpioReg::*;
use crate::mmio::regs::IrqReg::*;
use crate::mmio::regs::MmioReg::{Aux, Gpio, Irq};
use crate::mmio::Mmio;

pub fn init() {
    // Enable mini UART
    Mmio::write_reg(Aux(Enable), 1);

    // Disable transmitter and receiver during configuration
    Mmio::write_reg(Aux(MuCntl), 0);

    // Configure UART
    // Mmio::write_reg(Aux(MuIer), 0);
    Mmio::write_reg(Aux(MuIer), 3);
    Mmio::write_reg(Irq(S1), 1 << 29);

    Mmio::write_reg(Aux(MuLcr), 3); // Set the data size to 8 bit
    Mmio::write_reg(Aux(MuMcr), 0); // No auto flow control
    Mmio::write_reg(Aux(MuBaud), 270); // Set baud rate for 115200
    Mmio::write_reg(Aux(MuIir), 6); // No FIFOs (interrupt when receiver holds at least 1 byte

    // Map UART1 to GPIO pins
    let mut reg = Mmio::read_reg(Gpio(Gpfsel1));
    reg &= !((7 << 12) | (7 << 15)); // Clear existing settings for GPIO 14, 15
    reg |= (2 << 12) | (2 << 15); // Set to alt5 for mini UART
    Mmio::write_reg(Gpio(Gpfsel1), reg);

    // Disable pull-up/down for pins 14 and 15
    Mmio::write_reg(Gpio(Gppud), 0);
    Mmio::delay(300);
    Mmio::write_reg(Gpio(GppudClk0), (1 << 14) | (1 << 15));
    Mmio::delay(300);
    Mmio::write_reg(Gpio(GppudClk0), 0);

    // Enable the transmitter and receiver
    Mmio::write_reg(Aux(MuCntl), 3);
    unsafe {
        SND_IDX = 0;
        RCV_HEAD = 0;
        RCV_TAIL = 0;
    }
}

pub fn send(c: u8) {
    // Wait until we can send
    while Mmio::read_reg(Aux(MuLsr)) & 0x20 == 0 {
        Mmio::delay(1);
    }

    // Write the character to the buffer
    Mmio::write_reg(Aux(MuIo), c as u32);
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
    if Mmio::read_reg(Aux(MuLsr)) & 0x01 == 0 {
        return None;
    }
    // Read the character from the buffer
    Some((Mmio::read_reg(Aux(MuIo)) & 0xFF) as u8)
}

const BUFFER_SIZE: usize = 0x1000;
static mut SND_BUFFER: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];
static mut RCV_BUFFER: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];
static mut SND_IDX: usize = 0;
static mut RCV_HEAD: usize = 0;
static mut RCV_TAIL: usize = 0;

pub fn recv_async() -> Option<u8> {
    unsafe {
        if RCV_HEAD == RCV_TAIL {
            None
        } else {
            let c = RCV_BUFFER[RCV_HEAD];
            RCV_HEAD = (RCV_HEAD + 1) % BUFFER_SIZE;
            Some(c)
        }
    }
}

pub fn send_async(c: u8) {
    unsafe {
        SND_BUFFER[SND_IDX] = c;
        SND_IDX = SND_IDX + 1;
        while SND_IDX == BUFFER_SIZE - 1 {
            for i in 0..BUFFER_SIZE {
                send(SND_BUFFER[i]);
            }
            SND_IDX = 0;
        }
        Mmio::write_reg(Aux(MuIer), 0b11);
    }
}

pub fn handle_irq() {
    unsafe {
        for i in 0..SND_IDX {
            send(SND_BUFFER[i]);
        }
        SND_IDX = 0;

        loop {
            match recv_nb() {
                Some(c) => {
                    RCV_BUFFER[RCV_TAIL] = c;
                    RCV_TAIL = (RCV_TAIL + 1) % BUFFER_SIZE;
                }
                None => break,
            }
        }
        Mmio::write_reg(Aux(MuIer), 1);
    }
}
