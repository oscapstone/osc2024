use crate::{cpu::mailbox, os::stdio::*, println};
use core::{
    ptr::{read_volatile, write_volatile},
    usize,
};
use alloc::format;

const MAILBOX_BASE: usize = 0xFFFF_0000_3F00_B880;
const MAILBOX_READ: usize = MAILBOX_BASE + 0x00;
const MAILBOX_STATUS: usize = MAILBOX_BASE + 0x18;
const MAILBOX_WRITE: usize = MAILBOX_BASE + 0x20;

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
    let mut mailbox_addr = mailbox.mask(0xFFFF_FFFF) as u32;
    mailbox_addr = mailbox_addr & !0xF | (channel as u32 & 0xF);

    // println_now(format!("MBOX_ADDR {:x}", mailbox as usize).as_str());
    // println_now(format!("MBOX_ADDR {:x}", mailbox_addr).as_str());

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

    assert!(unsafe { read_volatile(mailbox.add(1)) } == REQUEST_SUCCEED);
}

pub fn get(tag: MailboxTag) -> (u32, u32) {
    let mut mailbox = [0u32; 8 + 4];
    let mut start_idx = mailbox.as_ptr() as usize;

    start_idx = (0x10 - (start_idx & 0xF)) / 4 % 4;

    assert_eq!(
        unsafe { mailbox.as_ptr().add(start_idx) } as usize % 16,
        0,
        "Mailbox buffer is not aligned to 16 bytes"
    );

    unsafe {
        let mailbox_ptr = mailbox.as_mut_ptr().add(start_idx);

        // println_now(format!("MBOX_ADDR {:x}", mailbox_ptr as usize).as_str());

        match tag {
            MailboxTag::GetBoardRevision => {
                write_volatile(mailbox_ptr.add(0), 7 * 4); // buffer size in bytes
                write_volatile(mailbox_ptr.add(3), 4); // maximum of request and response value buffer's length.
            }
            MailboxTag::GetArmMemory => {
                write_volatile(mailbox_ptr.add(0), 8 * 4);
                write_volatile(mailbox_ptr.add(3), 8);
            }
        }
        write_volatile(mailbox_ptr.add(1), REQUEST_CODE);
        write_volatile(mailbox_ptr.add(2), tag as u32); // tag identifier
        write_volatile(mailbox_ptr.add(4), TAG_REQUEST_CODE);
        write_volatile(mailbox_ptr.add(5), 0); // value buffer
        write_volatile(mailbox_ptr.add(6), END_TAG);

        mailbox_call(8, mailbox_ptr);
    }

    match tag {
        MailboxTag::GetBoardRevision => (mailbox[start_idx + 5], 0),
        MailboxTag::GetArmMemory => (mailbox[start_idx + 5], mailbox[start_idx + 6]),
    }
}
