use crate::driver::mailbox;

pub fn mbox_call(channel: u8, mbox: *mut u32) -> i32 {
    mailbox().call(channel, mbox);
    1
}
