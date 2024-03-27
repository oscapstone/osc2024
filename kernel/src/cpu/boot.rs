use alloc::vec::Vec;

use crate::cpu::{mailbox, uart};
use crate::os::shell;
use crate::os::stdio::*;
use core::arch::{asm, global_asm};

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() {
    uart::initialize();

    let a = mailbox::get(mailbox::MailboxTag::GetBoardRevision);
    print("Board revision: ");
    print_hex(a.0);

    let a: Vec<i32> = Vec::new();
    // let a = String::new();

    let a = mailbox::get(mailbox::MailboxTag::GetArmMemory);
    print("Memory base: ");
    print_hex(a.0);
    print("Memory size: ");
    print_hex(a.1);
    shell::start();
    loop {}
}
