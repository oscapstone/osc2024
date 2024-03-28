#![no_std]
#![no_main]

mod driver;
mod relocate;

use core::arch::asm;
use core::mem::transmute;
use core::{arch::global_asm, panic::PanicInfo};
use library::println;
use relocate::{Relocater, BINARY_START_ADDR, RELOCATE_ADDR};

global_asm! {include_str!("boot.s")}

static mut DEVICETREE_START_ADDR: usize = 0;

#[no_mangle]
pub unsafe fn _start_rust() -> ! {
    asm!("mov {}, x0", out(reg) DEVICETREE_START_ADDR);
    kernel_start();
}

unsafe fn jump() -> ! {
    let new_kernel_start = transmute::<*mut u8, fn() -> !>(BINARY_START_ADDR);
    asm!("mov x0, {}", in(reg) DEVICETREE_START_ADDR);
    new_kernel_start();
}

unsafe fn kernel_start() -> ! {
    unsafe {
        driver::init().unwrap();
    }
    println!("Bootloader started");
    println!(
        "Bootloader has been relocated to {:#08x}",
        RELOCATE_ADDR as usize
    );
    println!("Start loading kernel from mini UART input...");
    let relocater = Relocater {};
    relocater.run();
    println!("Read complete. Jump to new kernel start address.");
    jump();
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("{}", _info);
    loop {}
}
