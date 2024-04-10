use core::arch::{asm, global_asm};

use stdio::println;

global_asm!(include_str!("context_switch.S"));

#[no_mangle]
unsafe fn exception_handler() {
    asm!("save_all");
    println!("Exception occurred!");

    let mut el: u64;
    asm!("mrs {el}, CurrentEL", el = out(reg) el);
    println!("Current EL: {}", el >> 2);

    let mut spsr_el1: u64;
    asm!("mrs {spsr_el1}, spsr_el1", spsr_el1 = out(reg) spsr_el1);
    println!("spsr_el1: 0x{:x}", spsr_el1);

    let mut elr_el1: u64;
    asm!("mrs {elr_el1}, elr_el1", elr_el1 = out(reg) elr_el1);
    println!("elr_el1: 0x{:x}", elr_el1);

    let mut esr_el1: u64;
    asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);
    println!("esr_el1: 0x{:x}", esr_el1);

    asm!(
        "mrs {timer}, cntfrq_el0",
        "lsl {timer}, {timer}, 1", // 2 seconds
        "msr cntp_tval_el0, {timer}",
        timer = out(reg) _,
    );
    asm!("load_all");
}
