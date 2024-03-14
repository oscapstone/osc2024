#![feature(asm_const)]
#![feature(format_args_nl)]
#![feature(panic_info_message)]
#![feature(trait_alias)]
#![no_main]
#![no_std]

use core::{arch::global_asm, ptr::write_volatile};

mod bcm;
mod console;
mod panic;
mod print;
mod synchronization;

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

/// The Rust entry of the `kernel` binary.
///
/// The function is called from the assembly `_start` function.
///
/// # Safety
///
/// 1. boot.S link start entry to _start
/// 2. only core 0 would init bss, sp... and call _start_rust
/// 3. _start_rust call kernel_init
#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    bcm::hardware_init();
    crate::kernel_init();
}


/// Early kernel initialization.
/// # Safety
///
/// - Only a single core must be active and running this function.
unsafe fn kernel_init() -> ! {
    kernel_main()
}

const MAXCHAR: usize = 1000;

fn help() {
    println!("help    : print this help menu");
    println!("hello   : print Hello World!");
    println!("board   : print board rev");
    println!("reboot  : reboot this device");
}

unsafe fn reboot() {
    println!("Rebooting...");
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
        let r = PM_PASSWORD;
        write_volatile(PM_RSTC as *mut u32, r);
        write_volatile(PM_WDOG as *mut u32, r);
    }
}

unsafe fn interactiave_shell() -> ! {
    let mut array : [char; MAXCHAR] = ['\0'; MAXCHAR];
    let mut cnt = 0;

    loop {
        let c = bcm::UART.get_char();
        if c == '\r' {
            println!();
            match &array[0..6] {
                ['h', 'e', 'l', 'p', _] => {
                    help();
                }
                [ 'h', 'e', 'l', 'l', 'o', _] => {
                    println!("Hello World!");
                }
                [ 'r', 'e', 'b', 'o', 'o', 't'] => {
                    println!("Rebooting...");
                    reboot();
                }
                ['b', 'o', 'a', 'r', 'd', _] => {
                    let (board, _) = bcm::MAILBOX.get(bcm::mailbox::MailboxTag::GetBoardRevision);
                    let (mem0, mem1) = bcm::MAILBOX.get(bcm::mailbox::MailboxTag::GetArmMemory);
                    let (serial0, serial1) = bcm::MAILBOX.get(bcm::mailbox::MailboxTag::GetBoardSerial);
                    let (vc0, vc1) = bcm::MAILBOX.get(bcm::mailbox::MailboxTag::GetVcMemory);
                    println!("Board revision: {:x}", board);
                    println!("Board serial: {:x} {:x}", serial0, serial1);
                    println!("Arm memory(base, size): {:x}, {:x}", mem0, mem1);
                    println!("Vc memory(base, size):  {:x}, {:x}", vc0, vc1);
                }
                _ => {
                    if cnt > 0 {
                        println!("Unknown command: {:?}", &array[0..cnt]);
                        help();
                    }
                }
            }

            print!("\r# ");
            cnt = 0;
        } else {
            print!("{}", c);
            array[cnt] = c;
            if cnt < MAXCHAR - 1 {
                cnt += 1;
            } else {
                cnt = 0;
            }
        }
    }
}

unsafe fn kernel_main() -> ! {
    println!("[0] Hello from Rust!");
    println!("[1] run the simple shell");
    interactiave_shell()
}

#[no_mangle]
#[link_section = ".text._start_arguments"]
pub static BOOT_CORE_ID: u64 = 0;

global_asm!(include_str!("boot.S"),
CONST_CORE_ID_MASK = const 0b11
);
