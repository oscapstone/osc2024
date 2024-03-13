use crate::mmio::regs::MailboxReg::*;
use crate::mmio::regs::MmioReg::MailboxReg;
use crate::mmio::MMIO;
use crate::stdio;
use crate::utils::to_hex;

const MAILBOX_EMPTY: u32 = 0x4000_0000;
const MAILBOX_FULL: u32 = 0x8000_0000;

const CHANNEL_GPU: u32 = 8;

fn mailbox_read(channel: u32) -> u32 {
    loop {
        while MMIO::read_reg(MailboxReg(Status)) & MAILBOX_EMPTY != 0 {}
        let data = MMIO::read_reg(MailboxReg(Read));
        if (data & 0xF) == channel {
            return data;
        }
    }
}

fn mailbox_write(channel: u32, data: u32) {
    loop {
        while MMIO::read_reg(MailboxReg(Status)) & MAILBOX_FULL != 0 {}
        MMIO::write_reg(MailboxReg(Write), data | channel);
        return;
    }
}

fn mailbox_call(mailbox: &mut [u32; 7]) -> bool {
    let mailbox_ptr = mailbox.as_ptr() as u32;
    if mailbox_ptr & 0xF != 0 {
        // uart_puts(b"Mailbox pointer is not 16-byte aligned");
        return false;
    }

    mailbox_write(CHANNEL_GPU, mailbox_ptr);

    if mailbox_read(CHANNEL_GPU) != mailbox_ptr | CHANNEL_GPU {
        // uart_puts(b"Mailbox call failed");
        return false;
    }

    // uart_puts(b"Mailbox call succeeded");
    true
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
        stdio::write(b"Board revision: ");
        stdio::puts(&to_hex(mailbox[5]));
    } else {
        stdio::puts(b"Failed to get board revision");
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
        stdio::write(b"ARM memory base: ");
        stdio::puts(&to_hex(mailbox[5]));
        stdio::write(b"ARM memory size: ");
        stdio::puts(&to_hex(mailbox[6]));
    } else {
        stdio::puts(b"Failed to get ARM memory");
    }
}
