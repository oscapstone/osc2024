#![feature(asm_const)]
#![feature(format_args_nl)]
#![feature(panic_info_message)]
#![feature(trait_alias)]
#![no_main]
#![no_std]

use core::arch::global_asm;

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
    println!("Help    : print this help menu");
    println!("hello   : print Hello World!");
    println!("reboot  : reboot this device");
}

unsafe fn reboot() {
    println!("Rebooting...");
    // use core::arch::asm;
    // asm!("b 0x80000")
}

unsafe fn interactiave_shell() -> ! {
    let mut array : [char; MAXCHAR] = ['\0'; MAXCHAR];
    let mut cnt = 0;

    print!("# ");
    loop {
        let c = bcm::UART.get_char();
        if c == '\r' {
            println!();

            match &array[0..6] {
                &['h', 'e', 'l', 'p', _] => {
                    help();
                }
                &[ 'h', 'e', 'l', 'l', 'o', _] => {
                    println!("Hello World!");
                }
                &[ 'r', 'e', 'b', 'o', 'o', 't'] => {
                    println!("Rebooting...");
                    reboot();
                }
                _ => {
                    help();
                }
            }
            print!("# ");
            cnt = 0;
        } else {
            print!("{}", c);
            array[cnt] = c;
            cnt += 1;
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
