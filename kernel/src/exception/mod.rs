use core::{
    arch::{asm, global_asm},
    ptr::read_volatile,
};

use stdio::println;

global_asm!(include_str!("context_switch.S"));

#[no_mangle]
unsafe fn exception_handler() {
    disable_inturrupt();
    asm!("save_all");
    match read_volatile(0x4000_0060 as *const u32) {
        0x2 => {
            println!("Timer interrupt");
            let el: u64;
            asm!("mrs {el}, CurrentEL", el = out(reg) el);
            println!("Current EL: {}", el >> 2);

            let spsr_el1: u64;
            let elr_el1: u64;
            let esr_el1: u64;
            asm!("mrs {spsr_el1}, spsr_el1", spsr_el1 = out(reg) spsr_el1);
            asm!("mrs {elr_el1}, elr_el1", elr_el1 = out(reg) elr_el1);
            asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);

            println!("spsr_el1: 0x{:x}", spsr_el1);
            println!("elr_el1: 0x{:x}", elr_el1);
            println!("esr_el1: 0x{:x}", esr_el1);
            // Set the timer to fire again in 2 seconds
            asm!(
                "mrs {timer}, cntfrq_el0",
                "lsl {timer}, {timer}, 1", // 2 seconds
                "msr cntp_tval_el0, {timer}",
                timer = out(reg) _,
            );
        }
        0x3 => {
            println!("Mailbox interrupt");
        }
        _ => {
            println!("Unknown interrupt");
        }
    }
    asm!("load_all");
    enable_inturrupt();
}

#[inline(always)]
unsafe fn enable_inturrupt() {
    asm!("msr DAIFClr, 0xf");
}

#[inline(always)]
unsafe fn disable_inturrupt() {
    asm!("msr DAIFSet, 0xf");
}
