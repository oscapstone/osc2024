use crate::mmio::regs::MailboxReg::*;
use crate::mmio::regs::MmioReg::MailboxReg;
use crate::mmio::Mmio;

const MAILBOX_EMPTY: u32 = 0x4000_0000;
const MAILBOX_FULL: u32 = 0x8000_0000;

fn mailbox_read(channel: u8) -> u32 {
    loop {
        while Mmio::read_reg(MailboxReg(Status)) & MAILBOX_EMPTY != 0 {}
        let data = Mmio::read_reg(MailboxReg(Read));
        if (data & 0xF) as u8 == channel {
            return data;
        }
    }
}

fn mailbox_write(channel: u8, data: u32) {
    loop {
        while Mmio::read_reg(MailboxReg(Status)) & MAILBOX_FULL != 0 {}
        Mmio::write_reg(MailboxReg(Write), data | channel as u32);
        return;
    }
}

const CHANNEL_GPU: u8 = 8;

#[allow(dead_code)]
pub fn get_board_revision() -> u32 {
    let mut mailbox = [0; 7];
    mailbox[0] = 7 * 4; // Buffer size in bytes
    mailbox[1] = 0; // Request/response code
    mailbox[2] = 0x0001_0002; // Tag: Get board revision
    mailbox[3] = 4; // Buffer size in bytes
    mailbox[4] = 0; // Tag request code
    mailbox[5] = 0; // Value buffer
    mailbox[6] = 0x0000_0000; // End tag
    let mut mailbox = MailBox::new(&mailbox);
    assert!(mailbox.call(CHANNEL_GPU), "Failed to get board revision");
    mailbox.get(5)
}

#[allow(dead_code)]
pub fn get_arm_memory() -> (u32, u32) {
    let mut mailbox = [0; 8];
    mailbox[0] = 8 * 4; // Buffer size in bytes
    mailbox[1] = 0; // Request/response code
    mailbox[2] = 0x0001_0005; // Tag: Get ARM memory
    mailbox[3] = 8; // Buffer size in bytes
    mailbox[4] = 0; // Tag request code
    mailbox[5] = 0; // Value buffer
    mailbox[6] = 0; // Value buffer
    mailbox[7] = 0x0000_0000; // End tag
    let mut mailbox = MailBox::new(&mailbox);
    assert!(mailbox.call(CHANNEL_GPU), "Failed to get ARM memory");
    (mailbox.get(5), mailbox.get(6))
}

#[repr(C, align(16))]
pub struct MailBox {
    buffer: [u32; 36],
    pub len: u32,
}

impl MailBox {
    pub fn new(buf: &[u32]) -> Self {
        let mut buffer = [0; 36];
        let len = buf[0] as usize / 4;
        for i in 0..len {
            buffer[i] = buf[i] as u32;
        }
        MailBox {
            buffer,
            len: len as u32,
        }
    }

    pub fn call(&mut self, channel: u8) -> bool {
        let mailbox_ptr = self as *mut MailBox as u32;
        if mailbox_ptr & 0xF != 0 {
            panic!("Mailbox call failed");
        }
        mailbox_write(channel, mailbox_ptr);
        if mailbox_read(channel) != mailbox_ptr | channel as u32 {
            panic!("Mailbox call failed");
        }
        if self.buffer[1] != 0x8000_0000 {
            panic!("Mailbox call failed");
        }
        true
    }

    pub fn get(&self, index: usize) -> u32 {
        self.buffer[index]
    }
}
