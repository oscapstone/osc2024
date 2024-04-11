use core::{arch::{asm, global_asm}, ptr::read_volatile};
use super::super::cpu::uart;

global_asm!(include_str!("context_switching.s"));

#[no_mangle]
unsafe fn exception_handler_rust() {
    let mut freq: u64;
    let mut now: u64;
    // asm!(
    //     "mrs {freq}, cntfrq_el0",
    //     "mrs {now}, cntpct_el0",
    //     freq = out(reg) freq,
    //     now = out(reg) now,
    // );
    // println!("EXC_S: {}", now);

    // print exception level
    // let mut el: u64;
    // asm!("mrs {el}, CurrentEL", el = out(reg) el);
    // println!("Current EL: {}", el >> 2);

    match read_volatile(0x40000060 as *const u32) {
        0x2 => {
            println!("Timer interrupt");

            

            // Set the timer to fire again in 2 seconds
            asm!(
                "mrs {timer}, cntfrq_el0",
                "lsl {timer}, {timer}, 1", // 2 seconds
                "msr cntp_tval_el0, {timer}",
                timer = out(reg) _,
            );
        },
        0x0 => {
            let spsr_el1: u64;
            let elr_el1: u64;
            let esr_el1: u64;
            asm!("mrs {spsr_el1}, spsr_el1", spsr_el1 = out(reg) spsr_el1);
            asm!("mrs {elr_el1}, elr_el1", elr_el1 = out(reg) elr_el1);
            asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);
        
            println!("spsr_el1: 0x{:x}", spsr_el1);
            println!("elr_el1: 0x{:x}", elr_el1);
            println!("esr_el1: 0x{:x}", esr_el1);
        },
        // val => println!("Unknown interrupt: 0x{:x}", val),
        _ => (),
    }

    if read_volatile(0x3F21_5000 as *const u32) & 0x1 == 0x1 {
        // println!("mini UART interrupt");
        uart::irq_handler();
    }
    

}
