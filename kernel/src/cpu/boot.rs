use crate::cpu::{device_tree::DeviceTree, mailbox, uart};
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
    allocator::reserve(0x0000_0000 as *mut u8, 0x0000_1000); // Device reserved memory
    allocator::reserve(0x0003_0000 as *mut u8, 0x0004_0000); // Stack
    allocator::reserve(0x0007_5000 as *mut u8, 0x0000_0004); // CS counter
    allocator::reserve(0x0008_0000 as *mut u8, 0x0001_0000); // Code
    allocator::reserve(0x0800_0000 as *mut u8, 0x0100_0000); // Initramfs
    allocator::reserve(DeviceTree::get_device_tree_address(), 0x0100_0000); // Device Tree
    

    // Enable interrupts
    asm!("msr DAIFClr, 0xf");
    
    println!("Starting rust");
    
    let dt = DeviceTree::init();
    
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
