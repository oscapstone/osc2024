use crate::cpu::{device_tree::DeviceTree, mailbox, uart};
use crate::os::stdio::{print_hex_now, println_now};
use crate::os::{allocator, shell, timer};
use crate::println;
use alloc::boxed::Box;
use core::arch::{asm, global_asm};

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() {
    uart::initialize();
    timer::init();
    allocator::init();
    allocator::reserve(0x0000_0000 as *mut u8, 0x0000_1000); // Device reserved memory
    allocator::reserve(0x0003_0000 as *mut u8, 0x0004_0000); // Stack
    allocator::reserve(0x0007_5000 as *mut u8, 0x0000_0004); // CS counter
    allocator::reserve(0x0007_5100 as *mut u8, 0x0000_0004); // device tree address
    allocator::reserve(0x0008_0000 as *mut u8, 0x0004_0000); // Code
    allocator::reserve(0x0800_0000 as *mut u8, 0x0010_0000); // Initramfs
    allocator::reserve(DeviceTree::get_address(), 0x0100_0000); // Device Tree
    allocator::reserve(0x0880_0000 as *mut u8, 0x0780_0000); // Simple Allocator
 
    // Enable interrupts
    asm!("msr DAIFClr, 0xf");

    let dt = DeviceTree::init();

    println!("Device tree initialized");

    let initrd_start = match dt.get("linux,initrd-start") {
        Some(v) => {
            let mut val = 0u32;
            for i in 0..4 {
                val |= (v[i] as u32) << (i * 8);
            }
            val = val.swap_bytes();
            println!("Initrd start address: {:#X}", val);
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
