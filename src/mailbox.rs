use crate::mmio::regs::MailboxReg::*;
use crate::mmio::regs::MmioReg::MailboxReg;
use crate::mmio::MMIO;
use crate::stdio;

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

fn mailbox_call(mailbox: &mut [u32]) -> bool {
    let mailbox_ptr = mailbox.as_ptr() as u32;
    if mailbox_ptr & 0xF != 0 {
        // stdio::puts(b"Mailbox pointer is not 16-byte aligned");
        return false;
    }

    mailbox_write(CHANNEL_GPU, mailbox_ptr);

    if mailbox_read(CHANNEL_GPU) != mailbox_ptr | CHANNEL_GPU {
        stdio::puts(b"Mailbox call failed");
        return false;
    }

    true
}

pub fn get_board_revision() -> u32 {
    let mut mailbox = [
        7 * 4,       // Buffer size in bytes
        0,           // Request/response code
        0x0001_0002, // Tag: Get board revision
        4,           // Buffer size in bytes
        0,           // Tag request code
        0,           // Value buffer
        0x0000_0000, // End tag
    ];
    if !mailbox_call(&mut mailbox) {
        return 0;
    }
    mailbox[5]
}

pub fn get_arm_memory() -> (u32, u32) {
    let mut mailbox = [
        8 * 4,       // Buffer size in bytes
        0,           // Request/response code
        0x0001_0005, // Tag: Get ARM memory
        8,           // Buffer size in bytes
        0,           // Tag request code
        0,           // Value buffer
        0,           // Value buffer
        0x0000_0000, // End tag
    ];
    if !mailbox_call(&mut mailbox) {
        return (0, 0);
    }
    (mailbox[6], mailbox[7])
}
