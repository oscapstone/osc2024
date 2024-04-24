use crate::cpu::{mailbox, uart};
use crate::os::stdio::println_now;
use crate::os::{shell, timer, allocator};
use crate::println;
use core::arch::{asm, global_asm};

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() {
    // crate::os::allocator::ALLOCATOR.init();
    uart::initialize();
    timer::init();
    allocator::init();

    // Enable interrupts
    asm!("msr DAIFClr, 0xf");
    
    println!("Starting rust");
    
    let dt = super::device_tree::DeviceTree::init();
    
    println!("Device tree initialized");
    
    let initrd_start = match dt.get("linux,initrd-start") {
        Some(v) => {
            let mut val = 0u32;
            for i in 0..4 {
                val |= (v[i] as u32) << (i * 8);
            }
            val = val.swap_bytes();
            println!("Initrd start: {:#X}", val);
            val
        }
        None => {
            println!("No initrd");
            0
        }
    };
    // let initrd_start = 0x8000000;

    print_information();

    shell::start(initrd_start);
    loop {}
}

fn print_information() {
    let board_revision = mailbox::get(mailbox::MailboxTag::GetBoardRevision);
    println!("Board revision: {:#010X}", board_revision.0);

    let memory = mailbox::get(mailbox::MailboxTag::GetArmMemory);
    println!("Memory base: {:#010X}", memory.0);
    println!("Memory size: {:#010X}", memory.1);
    println!("Boot time: {} ms", timer::get_time_ms());
}
