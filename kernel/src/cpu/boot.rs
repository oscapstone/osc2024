use core::arch::{asm, global_asm};
use crate::os::stdio::{print, print_hex};
use crate::os::shell;

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() { 
    crate::cpu::uart::initialize();
    asm!("nop");
    let a = crate::cpu::mailbox::get(crate::cpu::mailbox::MailboxTag::GetBoardRevision);
    asm!("nop");
    print("Board revision: ");
    print_hex(a.0);

    let a = crate::cpu::mailbox::get(crate::cpu::mailbox::MailboxTag::GetArmMemory);
    print("Memory base: ");
    print_hex(a.0);
    print("Memory size: ");
    print_hex(a.1);
    shell::start();
    loop {};
}