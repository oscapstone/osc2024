use crate::mmio::regs::MailboxReg::*;
use crate::mmio::regs::MmioReg::MailboxReg;
use crate::mmio::Mmio;
use core::ptr::{read_volatile, write_volatile};

const MAILBOX_EMPTY: u32 = 0x4000_0000;
const MAILBOX_FULL: u32 = 0x8000_0000;

const CHANNEL_GPU: u32 = 8;

#[allow(dead_code)]
fn mailbox_read(channel: u32) -> u32 {
    loop {
        while Mmio::read_reg(MailboxReg(Status)) & MAILBOX_EMPTY != 0 {}
        let data = Mmio::read_reg(MailboxReg(Read));
        if (data & 0xF) == channel {
            return data;
        }
    }
}

#[allow(dead_code)]
fn mailbox_write(channel: u32, data: u32) {
    loop {
        while Mmio::read_reg(MailboxReg(Status)) & MAILBOX_FULL != 0 {}
        Mmio::write_reg(MailboxReg(Write), data | channel);
        return;
    }
}

#[allow(dead_code)]
fn mailbox_call(mailbox: &mut [u32]) -> bool {
    let mailbox_ptr_org = mailbox.as_ptr() as u32;
    // shift mailbox to align 16 bytes
    let mailbox_ptr = (mailbox_ptr_org + 0xF) & !0xF;
    for i in 0..mailbox.len() {
        unsafe {
            write_volatile(
                (mailbox_ptr + (mailbox.len() as u32 - 1 - i as u32) * 4 as u32) as *mut u32,
                mailbox[mailbox.len() - 1 - i],
            );
        }
    }
    if mailbox_ptr & 0xF != 0 {
        panic!("Mailbox call failed");
    }
    debug_assert!(mailbox_ptr & 0xF == 0, "Mailbox call failed");
    mailbox_write(CHANNEL_GPU, mailbox_ptr);

    if mailbox_read(CHANNEL_GPU) != mailbox_ptr | CHANNEL_GPU {
        panic!("Mailbox call failed");
    }

    for i in 0..mailbox.len() {
        unsafe {
            write_volatile(
                &mut mailbox[i],
                read_volatile((mailbox_ptr + (i * 4) as u32) as *const u32),
            );
        }
    }
    true
}

#[allow(dead_code)]
pub fn get_board_revision() -> u32 {
    let mut mailbox = [0u32; 16];
    mailbox[0] = 7 * 4; // Buffer size in bytes
    mailbox[1] = 0; // Request/response code
    mailbox[2] = 0x0001_0002; // Tag: Get board revision
    mailbox[3] = 4; // Buffer size in bytes
    mailbox[4] = 0; // Tag request code
    mailbox[5] = 0; // Value buffer
    mailbox[6] = 0x0000_0000; // End tag

    for i in 7..16 {
        mailbox[i] = 0;
    }
    if !mailbox_call(&mut mailbox) {
        panic!("Failed to get board revision");
    }
    mailbox[5]
}

#[allow(dead_code)]
pub fn get_arm_memory() -> (u32, u32) {
    let mut mailbox = [0u32; 16];
    mailbox[0] = 8 * 4; // Buffer size in bytes
    mailbox[1] = 0; // Request/response code
    mailbox[2] = 0x0001_0005; // Tag: Get ARM memory
    mailbox[3] = 8; // Buffer size in bytes
    mailbox[4] = 0; // Tag request code
    mailbox[5] = 0; // Value buffer
    mailbox[6] = 0; // Value buffer
    mailbox[7] = 0x0000_0000; // End tag

    if !mailbox_call(&mut mailbox) {
        panic!("Failed to get ARM memory");
    }
    (mailbox[5], mailbox[6])
}
