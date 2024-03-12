use crate::mmio::regs::MailboxReg::*;
use crate::mmio::regs::MmioReg;
use crate::mmio::MMIO;
use crate::uart;

use utils::to_hex;

const MAILBOX_EMPTY: u32 = 0x4000_0000;
const MAILBOX_FULL: u32 = 0x8000_0000;

const CHANNEL_GPU: u32 = 8;

fn mailbox_call(mailbox: &mut [u32]) -> bool {
    let message_address = mailbox.as_ptr() as u32 & !0xF; // Ensure alignment and remove the lower 4 bits
    let message_with_channel = message_address | CHANNEL_GPU;

    // Wait for MAILBOX to be not full
    while MMIO::read_reg(MmioReg::MailboxReg(Status)) & MAILBOX_FULL != 0 {}

    // Write the message address to MAILBOX_WRITE with channel
    MMIO::write_reg(MmioReg::MailboxReg(Write), message_with_channel);

    loop {
        // Wait for MAILBOX to have something for us
        while MMIO::read_reg(MmioReg::MailboxReg(Status)) & MAILBOX_EMPTY != 0 {}

        let response = MMIO::read_reg(MmioReg::MailboxReg(Read));
        if (response & 0xF) == CHANNEL_GPU && (response & !0xF) == message_address {
            return mailbox[1] == 0;
        }
    }
}

pub fn get_board_revision() {
    let mut mailbox: [u32; 7] = [
        7 * 4,      // buffer size in bytes
        0x00000000, // REQUEST_CODE
        0x00010002, // GET_BOARD_REVISION tag identifier
        4,          // value buffer size
        0x00000000, // TAG_REQUEST_CODE (request)
        0,          // value buffer (response will be placed here)
        0x00000000, // END_TAG
    ];

    if mailbox_call(&mut mailbox) {
        uart::uart_write(b"Board revision: ");
        uart::uart_puts(&to_hex(mailbox[5]));
    } else {
        uart::uart_puts(b"Failed to get board revision");
    }
}

pub fn get_arm_memory() {
    let mut mailbox: [u32; 7] = [
        7 * 4,      // buffer size in bytes
        0x00000000, // REQUEST_CODE
        0x00010005, // GET_ARM_MEMORY tag identifier
        8,          // value buffer size
        0x00000000, // TAG_REQUEST_CODE (request)
        0,          // value buffer (response will be placed here)
        0x00000000, // END_TAG
    ];

    if mailbox_call(&mut mailbox) {
        uart::uart_write(b"ARM memory base: ");
        uart::uart_puts(&to_hex(mailbox[5]));
        uart::uart_write(b"ARM memory size: ");
        uart::uart_puts(&to_hex(mailbox[6]));
    } else {
        uart::uart_puts(b"Failed to get ARM memory");
    }
}
