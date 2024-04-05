use crate::cpu::{mailbox, uart};
use crate::os::shell;
use crate::println;
use core::arch::{asm, global_asm};

global_asm!(include_str!("boot.s"));

#[no_mangle]
pub unsafe fn _start_rust() {
    // crate::os::allocator::ALLOCATOR.init();
    uart::initialize();
    println!("Starting rust");
    
    // test_allocator();

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

    let a = mailbox::get(mailbox::MailboxTag::GetBoardRevision);
    println!("Board revision: {:#010X}", a.0);

    let a = mailbox::get(mailbox::MailboxTag::GetArmMemory);
    println!("Memory base: {:#010X}", a.0);
    println!("Memory size: {:#010X}", a.1);
    shell::start(initrd_start);
    loop {}
}

/*
fn test_allocator() {
    print("Testing allocator");
    for i in 0..1000 {
        if i % 10 == 0 {
            print(".");
        }
        let mut v1: Vec<u32> = Vec::new();
        let mut v2: Vec<u32> = Vec::new();
        let mut v3: Vec<u32> = Vec::new();
        for j in 0..100000 {
            v1.push(j);
            v2.push(j);
            v3.push(j);
        }
        for i in 0..100000 {
            if v1[i] != i as u32 {
                println("Error in allocator");
                loop {}
            }
            if v2[i] != i as u32 {
                println("Error in allocator");
                loop {}
            }
            if v3[i] != i as u32 {
                println("Error in allocator");
                loop {}
            }
        }
    }
    println("\rAllocator test passed");
}
*/