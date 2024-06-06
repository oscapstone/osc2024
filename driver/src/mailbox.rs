const MMIO_BASE: u32 = 0x3f000000;
const MAILBOX_BASE: u32 = MMIO_BASE + 0xb880;

const MAILBOX_READ: u32 = MAILBOX_BASE;
const MAILBOX_STATUS: u32 = MAILBOX_BASE + 0x18;
const MAILBOX_WRITE: u32 = MAILBOX_BASE + 0x20;

const MAILBOX_EMPTY: u32 = 0x40000000;
const MAILBOX_FULL: u32 = 0x80000000;

use core::ptr::{read_volatile, write_volatile};

#[no_mangle]
#[inline(never)]
pub fn mailbox_call(ch:u32, mailbox: *const u32) {
    // check if the mailbox is full
    unsafe {
        let mut status: u32;
        loop {
            status = read_volatile(MAILBOX_STATUS as *const u32);
            if (status & MAILBOX_FULL) == 0 {
                // mailbox is not full, we can write the message
                break;
            }
        }
        // Combine the message address (upper 28 bits) with channel number (lower 4 bits)
        let channel = ch;
        let message_address = mailbox as u32;
        let combined_address = (message_address & 0xFFFFFFF0) | (channel & 0xF);
        write_volatile(MAILBOX_WRITE as *mut u32, combined_address as u32);

        // check if the mailbox is empty
        loop {
            status = read_volatile(MAILBOX_STATUS as *const u32);
            if (status & MAILBOX_EMPTY) == 0 {
                // mailbox is not empty, we can read the message
                break;
            }
        }
        if read_volatile(MAILBOX_READ as *const u32) != (message_address as u32) {}
    }
}



const GET_BOARD_REVISION: u32 = 0x00010002;
const REQUEST_CODE: u32 = 0x00000000;
const REQUEST_SUCCEED: u32 = 0x80000000;
const REQUEST_FAILED: u32 = 0x80000001;
const TAG_REQUEST_CODE: u32 = 0x00000000;
const END_TAG: u32 = 0x00000000;

pub fn get_board_revisioin() -> u32 {
    let mut mailbox: [u32; 32] = [0; 32];
    let idx_offset: usize = (0x10 - mailbox.as_ptr() as u32 & 0xf) as usize;

    mailbox[0 + idx_offset] = 7 * 4;
    mailbox[1 + idx_offset] = REQUEST_CODE;
    mailbox[2 + idx_offset] = GET_BOARD_REVISION;
    mailbox[3 + idx_offset] = 4;
    mailbox[4 + idx_offset] = TAG_REQUEST_CODE;
    mailbox[5 + idx_offset] = 0;
    mailbox[6 + idx_offset] = END_TAG;

    mailbox_call(8,(mailbox[idx_offset..(idx_offset + 7)]).as_ptr());
    mailbox[5]
}

// Get ARM memory base address and size using mailbox
const GET_ARM_MEMORY: u32 = 0x00010005;
pub fn get_arm_memory() -> (u32, u32) {
    let mut mailbox: [u32; 32] = [0; 32];
    let idx_offset: usize = (0x10 - mailbox.as_ptr() as u32 & 0xf) as usize;

    mailbox[0 + idx_offset] = 8 * 4;
    mailbox[1 + idx_offset] = REQUEST_CODE;
    mailbox[2 + idx_offset] = GET_ARM_MEMORY;
    mailbox[3 + idx_offset] = 8;
    mailbox[4 + idx_offset] = TAG_REQUEST_CODE;
    mailbox[5 + idx_offset] = 0;
    mailbox[6 + idx_offset] = 0;
    mailbox[7 + idx_offset] = END_TAG;

    mailbox_call(8, mailbox[idx_offset..(idx_offset + 8)].as_ptr());
    (mailbox[5], mailbox[6])
}
