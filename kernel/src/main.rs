#![feature(asm_const)]
#![feature(format_args_nl)]
#![feature(panic_info_message)]
#![feature(trait_alias)]
#![no_main]
#![no_std]

use core::ptr::write_volatile;

mod bsp;
mod console;
mod cpu;
mod driver;
mod panic_wait;
mod print;
mod synchronization;
mod mbox;

mod arrsting;

use arrsting::ArrString;

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

/// The main function running after the early init.
unsafe fn kernel_main() -> ! {
    use console::console;

    println!(
        "[0] {} version {}",
        env!("CARGO_PKG_NAME"),
        env!("CARGO_PKG_VERSION")
    );
    println!("[1] Booting on: {}", bsp::board_name());

    println!("[2] Drivers loaded:");
    driver::driver_manager().enumerate();

    println!("[3] Chars written: {}", console().chars_written());
    println!("[4] Echoing input now");

    let msg_buf_exceed = "[system] Buf size limit exceed, reset buf";

    let msg_help = "help\t: print this help menu\r\nhello\t: print Hello World!\r\nreboot\t: reboot the device\r\ninfo\t: show the device info";
    let msg_hello_world = "HelloWorld!";
    let msg_not_found = "Command not found";
    let msg_reboot = "Rebooting...";

    let arr_help = ArrString::new("help");
    let arr_hello = ArrString::new("hello");
    let arr_reboot = ArrString::new("reboot");
    let arr_info = ArrString::new("info");

    let mut buf = ArrString::new("");

    // Discard any spurious received characters before going into echo mode.
    console().clear_rx();

    println!("TEST VER 0.0.2\r\n");
    print!("{}\r\n#", msg_help);

    loop {
        let c = console().read_char();
        console().write_char(c);

        if c == '\n' {
            print!("\r");
            if buf == arr_help {
                println!("{}", msg_help);
            } else if buf == arr_hello {
                println!("{}", msg_hello_world);
            } else if buf == arr_reboot {
                println!("{}", msg_reboot);
                reboot();
            } else if buf == arr_info {
                println!("BoardVersion: {:x}", mbox::mbox().get_board_revision());
                // println!("BoardVersion: {:x}", bsp::driver::MBOX.get_board_revision());
                println!(
                    "RAM: {} {}",
                    mbox::mbox().get_arm_memory().0,
                    mbox::mbox().get_arm_memory().1
                );
            } else {
                println!("{}", msg_not_found);
            }

            buf.clean_buf();
            print!("#");
            continue;

            // arrsting::arrstrcmp(buf, help);
        } else if buf.get_len() == 1024 {
            buf.clean_buf();
            println!("{}", msg_buf_exceed);
            print!("#");
            continue;
        } else {
            buf.push_char(c);
        }
    }
}

unsafe fn reboot() {
    reset(100);
}

const PM_PASSWORD: u32 = 0x5a000000;
const PM_RSTC: u32 = 0x3F10_001C;
const PM_WDOG: u32 = 0x3F10_0024;

pub fn reset(tick: u32) {
    unsafe {
        let mut r = PM_PASSWORD | 0x20;
        write_volatile(PM_RSTC as *mut u32, r);
        r = PM_PASSWORD | tick;
        write_volatile(PM_WDOG as *mut u32, r);
    }
}

pub fn cancel_reset() {
    unsafe {
        let r = PM_PASSWORD | 0;
        write_volatile(PM_RSTC as *mut u32, r);
        write_volatile(PM_WDOG as *mut u32, r);
    }
}
