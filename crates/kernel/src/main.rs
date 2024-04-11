#![feature(asm_const)]
#![feature(error_in_core)]
#![no_std]
#![no_main]

extern crate alloc;

mod allocator;
mod boot;
mod cpio;
mod devicetree;
mod driver;
mod shell;

use cpio::CpioArchive;
use devicetree::DeviceTree;
use panic_wait as _;
use shell::ShellCommand;
use small_std::println;

use crate::devicetree::DeviceTreeEntryValue;

const INITRD_DEVICETREE_NODE: &str = "chosen";
const INITRD_DEVICETREE_PROP: &str = "linux,initrd-start";

unsafe fn kernel_init() -> ! {
    if let Err(e) = driver::register_drivers() {
        panic!("Failed to initialize driver subsystem: {}", e);
    }

    device::driver::driver_manager().init_drivers();

    // Finnaly go from unsafe to safe 🎉
    main()
}

fn main() -> ! {
    println!(
        "[0] {} version {}",
        env!("CARGO_PKG_NAME"),
        env!("CARGO_PKG_VERSION")
    );

    println!("[1] Drivers loaded:");
    device::driver::driver_manager().enumerate();

    println!("[2] DTB loaded at: {:#x}", unsafe {
        boot::DEVICETREE_START_ADDR
    });

    let mut cpio_start_addr = 0;

    let devicetree = unsafe { DeviceTree::new(boot::DEVICETREE_START_ADDR) };
    if let Err(e) = devicetree.traverse(|node, props| {
        if node != INITRD_DEVICETREE_NODE {
            return;
        }
        for prop in props {
            let prop = prop.unwrap();
            if prop.name != INITRD_DEVICETREE_PROP {
                continue;
            }
            match prop.value {
                DeviceTreeEntryValue::U32(v) => cpio_start_addr = v as usize,
                DeviceTreeEntryValue::U64(v) => cpio_start_addr = v as usize,
                DeviceTreeEntryValue::String(v) => println!("invalid initrd start address: {}", v),
                DeviceTreeEntryValue::Bytes(v) => println!("invalid initrd start address: {:?}", v),
            }
        }
    }) {
        println!("Failed to parse devicetree: {}", e);
    };

    if cpio_start_addr == 0 {
        println!("No initrd found. Halting...");
        loop {}
    }
    println!("[3] CPIO loaded at: {:#x}", cpio_start_addr);

    println!("[4] Echoing input now");

    let cpio = unsafe { CpioArchive::new(cpio_start_addr) };
    let commands: &[&dyn ShellCommand] = &[
        &shell::commands::HelloCommand,
        &shell::commands::RebootCommand,
        &shell::commands::InfoCommand,
        &shell::commands::LsCommand::new(&cpio),
        &shell::commands::CatCommand::new(&cpio),
    ];
    let mut shell = shell::Shell::new(commands);
    shell.run_loop();
}
