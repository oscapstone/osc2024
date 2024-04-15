use super::{super::{
    cpu::uart,
    os::timer,
}, stdio::println_now};
use core::{
    arch::{asm, global_asm},
    ptr::read_volatile,
};

global_asm!(include_str!("context_switching.s"));

#[no_mangle]
unsafe fn exception_handler_rust() {
    // println!("Exception handler");
    let interrupt_source = read_volatile(0x40000060 as *const u32);

    if interrupt_source & 0x2 > 0 {
        timer::irq_handler();
    }
    if interrupt_source == 0 {
        let spsr_el1: u64;
        let elr_el1: u64;
        let esr_el1: u64;
        asm!("mrs {spsr_el1}, spsr_el1", spsr_el1 = out(reg) spsr_el1);
        asm!("mrs {elr_el1}, elr_el1", elr_el1 = out(reg) elr_el1);
        asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);

        println!("spsr_el1: 0x{:x}", spsr_el1);
        println!("elr_el1: 0x{:x}", elr_el1);
        println!("esr_el1: 0x{:x}", esr_el1);
    }

    if read_volatile(0x3F21_5000 as *const u32) & 0x1 == 0x1 {
        // println!("mini UART interrupt");
        uart::irq_handler();
    }
}
