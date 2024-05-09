#![feature(asm_const)]
#![feature(format_args_nl)]
#![feature(panic_info_message)]
#![feature(trait_alias)]
#![no_main]
#![no_std]

mod arrsting;
mod bsp;
mod console;
mod cpu;
mod driver;
mod mbox;
mod panic_wait;
mod power;
mod print;
mod shell;
mod synchronization;

/// Early init code.
///
/// # Safety
///
/// - Only a single core must be active and running this function.
/// - The init calls in this function must appear in the correct order.
unsafe fn kernel_init() -> ! {
    // Initialize the BSP driver subsystem.
    if let Err(x) = bsp::driver::init() {
        panic!("Error initializing BSP driver subsystem: {}", x);
    }

    // Initialize all device drivers.
    driver::driver_manager().init_drivers();
    // println! is usable from here on.

    // Transition from unsafe to safe.
    kernel_main()
}

// const PI_LOADER_LOGO: &str = r#"
//    ___ _   __                 _           
//   / _ (_) / /  ___   __ _  __| | ___ _ __ 
//  / /_)/ |/ /  / _ \ / _` |/ _` |/ _ \ '__|
// / ___/| / /__| (_) | (_| | (_| |  __/ |   
// \/    |_\____/\___/ \__,_|\__,_|\___|_|   
// "#;

/// The main function running after the early init.
fn kernel_main() -> ! {
    use console::console;

    // println!(
    //     "[0] {} version {}",
    //     env!("CARGO_PKG_NAME"),
    //     env!("CARGO_PKG_VERSION")
    // );
    // println!("[1] Booting on: {}", bsp::board_name());

    // println!("[2] Drivers loaded:");
    // driver::driver_manager().enumerate();

    // println!("[3] Chars written: {}", console().chars_written());
    // println!("[4] Echoing input now");

    // println!("{}", PI_LOADER_LOGO);
    // println!("BoardName: {}", bsp::board_name());
    // println!("BoardVersion: {:x}", mbox::mbox().get_board_revision());
    // println!(
    //     "RAM: {} {}",
    //     mbox::mbox().get_arm_memory().0,
    //     mbox::mbox().get_arm_memory().1
    // );
    // println!();
    // print!("3");
    // console().flush();

    // Discard any spurious received characters before starting with the loader protocol.
    // console().clear_rx();

    // loop {
    //     println!("OK!");
    // }

    // let dtb_addr = 0x50000 as *const u8;
    // let dtb_addr = unsafe { core::ptr::read_volatile(dtb_addr as *const u32) };

    // loop {
    //     println!("OK!");
    //     println!("DTB address: {:#x}", dtb_addr);
    // }

    // Read the binary's size.
    let mut size: u32 = u32::from(console().read_char() as u8);
    size |= u32::from(console().read_char() as u8) << 8;
    size |= u32::from(console().read_char() as u8) << 16;
    size |= u32::from(console().read_char() as u8) << 24;

    // Trust it's not too big.
    // console().write_char('O');
    // console().write_char('K');

    let kernel_addr: *mut u8 = bsp::memory::board_default_load_addr() as *mut u8;

    // println!("image size: {}", size);

    unsafe {
        // Read the kernel byte by byte.
        let mut checksum: u16 = 0;
        for i in 0..size {
            let c = console().read_char();
            checksum = (checksum % 100 + c as u16 % 100) % 100;

            core::ptr::write_volatile(kernel_addr.offset(i as isize), c as u8);
            // if i % 1024 == 0{
            //     println!("pos:{} data:{} checksum:{}",i,c as u16, checksum);
            // }
        }
    }
    // unsafe {
    //     let kernel = core::slice::from_raw_parts_mut(0x80000 as *mut u8, size as usize);
    //     for i in 0..size {
    //         let c = console().read_char();
    //         kernel[i as usize] = c as u8;
    //         println!("pos:{} data:{}",i,c as u32);
    //     }
    // }

    println!("[ML] Loaded! Executing the payload now");
    console().flush();

    // Use black magic to create a function pointer.
    let kernel: fn() -> ! = unsafe { core::mem::transmute(kernel_addr) };

    // println!("[ML] Try to jump to bootloader");
    // Jump to loaded kernel!
    kernel();

    // shell::start_shell();
}
