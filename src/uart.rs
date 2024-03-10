use mmio::AuxReg::*;
use mmio::GpioReg::*;
use mmio::MmioReg::*;
use mmio::MMIO;

pub unsafe fn uart_init() {
    // Enable mini UART
    MMIO::write_reg(Aux(Enable), 1);

    // Disable transmitter and receiver during configuration
    MMIO::write_reg(Aux(MuCntl), 0);

    // Configure UART
    MMIO::write_reg(Aux(MuLcr), 3); // Set the data size to 8 bit
    MMIO::write_reg(Aux(MuMcr), 0); // No auto flow control
    MMIO::write_reg(Aux(MuBaud), 270); // Set baud rate for 115200
    MMIO::write_reg(Aux(MuIir), 6); // No FIFO

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
    MMIO::write_reg(Aux(MuIer), 0);
}

pub unsafe fn uart_send(c: u8) {
    // Wait until we can send
    while MMIO::read_reg(Aux(MuLsr)) & 0x20 == 0 {
        core::arch::asm!("nop");
    }

    // Write the character to the buffer
    MMIO::write_reg(Aux(MuIo), c as u32);

    // If the character is newline, also send carriage return
    if c == b'\n' {
        uart_send(b'\r');
    }
}

pub unsafe fn uart_recv() -> u8 {
    // Wait until we can receive
    while MMIO::read_reg(Aux(MuLsr)) & 0x01 == 0 {
        core::arch::asm!("nop");
    }

    // Read the character from the buffer
    let c: u8 = MMIO::read_reg(Aux(MuIo)) as u8;
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

#[allow(dead_code)]
pub unsafe fn uart_write(buf: &[u8]) {
    for &c in buf {
        uart_send(c);
    }
}

#[allow(dead_code)]
pub unsafe fn uart_puts(buf: &[u8]) {
    for &c in buf {
        if c == 0 {
            break;
        }
        uart_send(c);
    }
    uart_send(b'\n');
}

#[allow(dead_code)]
pub unsafe fn uart_gets(buf: &mut [u8]) -> usize {
    let mut i = 0;
    loop {
        let input = uart_recv();
        match input {
            b'\n' => {
                if i < buf.len() {
                    uart_send(b'\n');
                    buf[i] = 0;
                    break;
                }
            }
            b'\x08' | b'\x7f' => {
                if i < buf.len() && i > 0 {
                    uart_send(b'\x08');
                    uart_send(b' ');
                    uart_send(b'\x08');
                    i -= 1;
                    buf[i] = 0;
                }
            }
            _ => {
                if i < buf.len() {
                    uart_send(input);
                    buf[i] = input;
                    i += 1;
                }
            }
        }
    }
    i
}
