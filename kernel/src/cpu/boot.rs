use core::arch::{asm, global_asm};
use crate::os::stdio::{print, print_char_64, print_dec, print_hex, println};
use crate::os::shell;
use crate::os::file_system::cpio;
use crate::cpu::{uart, mailbox};

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() { 
    uart::initialize();

    // let init_ram_file = cpio::CpioArchive::load(0x800_0000 as *const u8);
    // // println(init_ram_file.get_file_name());
    // print_dec(init_ram_file.get_num_files());


    let a = mailbox::get(mailbox::MailboxTag::GetBoardRevision);
    print("Board revision: ");
    print_hex(a.0);

    let a = mailbox::get(mailbox::MailboxTag::GetArmMemory);
    print("Memory base: ");
    print_hex(a.0);
    print("Memory size: ");
    print_hex(a.1);
    shell::start();
    loop {};
}