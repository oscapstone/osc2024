use core::{ptr::{read_volatile, write_volatile}, usize};
use crate::{os::stdio::*, println};

const MAILBOX_BASE: u32 = 0x3F00_B880;
const MAILBOX_READ: u32 = MAILBOX_BASE + 0x00;
const MAILBOX_STATUS: u32 = MAILBOX_BASE + 0x18;
const MAILBOX_WRITE: u32 = MAILBOX_BASE + 0x20;

const MAILBOX_EMPTY: u32 = 0x4000_0000;
const MAILBOX_FULL: u32 = 0x8000_0000;

const GET_BOARD_REVISION: u32 = 0x0001_0002;
const REQUEST_CODE: u32 = 0x0000_0000;
const REQUEST_SUCCEED: u32 = 0x8000_0000;
const REQUEST_FAILED: u32 = 0x8000_0001;
const TAG_REQUEST_CODE: u32 = 0x0000_0000;
const END_TAG: u32 = 0x0000_0000;

#[derive(Copy, Clone)]
pub enum MailboxTag {
    GetBoardRevision = 0x0001_0002,
    GetArmMemory = 0x0001_0005,
}

#[inline(never)]
pub fn mailbox_call(channel: u8, mailbox: *mut u32) {
    let mut mailbox_addr = mailbox as u32;
    mailbox_addr = mailbox_addr & !0xF | (channel as u32 & 0xF);

    unsafe {
        while (read_volatile(MAILBOX_STATUS as *const u32) & MAILBOX_FULL) != 0 {}
        write_volatile(MAILBOX_WRITE as *mut u32, mailbox_addr);
        
        loop {
            while (read_volatile(MAILBOX_STATUS as *const u32) & MAILBOX_EMPTY) != 0 {}
            let data = read_volatile(MAILBOX_READ as *const u32);
            if data == mailbox_addr {
                break;
            }
        }
    }
}

pub fn get(tag: MailboxTag) -> (u32, u32) {
    let mut mailbox = [0u32; 8 + 4];
    let mut start_idx = mailbox.as_ptr() as usize;

    start_idx = (0x10 - (start_idx & 0xF)) / 4 % 4;

    match tag {
        MailboxTag::GetBoardRevision => {
            mailbox[start_idx + 0] = 7 * 4; // buffer size in bytes
            mailbox[start_idx + 3] = 4; // maximum of request and response value buffer's length.
        }
        MailboxTag::GetArmMemory => {
            mailbox[start_idx + 0] = 8 * 4; // buffer size in bytes
            mailbox[start_idx + 3] = 8; // maximum of request and response value buffer's length.
        }
    }
    mailbox[start_idx + 1] = REQUEST_CODE;
    mailbox[start_idx + 2] = tag as u32; // tag identifier
    mailbox[start_idx + 4] = TAG_REQUEST_CODE;
    mailbox[start_idx + 5] = 0; // value buffer
    mailbox[start_idx + 6] = END_TAG;

    unsafe {
        mailbox_call(8, mailbox.as_mut_ptr().add(start_idx) as *mut u32);

    }

    match tag {
        MailboxTag::GetBoardRevision => {
            (mailbox[start_idx + 5], 0)
        }
        MailboxTag::GetArmMemory => {
            (mailbox[start_idx + 5], mailbox[start_idx + 6])
        }
        
    }
}
