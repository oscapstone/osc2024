mod handlers;

use core::{
    arch::{asm, global_asm},
    ptr::read_volatile,
};
use driver::mmio::{regs::AuxReg, regs::MmioReg, Mmio};
use stdio::{debug, println};

global_asm!(include_str!("context_switch.S"));
global_asm!(include_str!("exception_table.S"));

#[no_mangle]
unsafe fn unknown_exception_handler() {
    disable_interrupt();
    let esr_el1: u64;
    let elr_el1: u64;
    asm!(
        "mrs {0}, esr_el1", out(reg) esr_el1,
    );
    asm!(
        "mrs {0}, elr_el1", out(reg) elr_el1,
    );
    debug!("Unknown exception");
    debug!("ESR_EL1: 0x{:x}", esr_el1);
    debug!("ELR_EL1: 0x{:x}", elr_el1);
    panic!("Unknown exception");
}

#[allow(dead_code)]
#[inline(always)]
pub unsafe fn enable_interrupt() {
    // println!("=========================================== Enabling interrupts");
    asm!("msr DAIFClr, 0xf");
}

#[allow(dead_code)]
#[inline(always)]
pub unsafe fn disable_interrupt() {
    // println!("=========================================== Disabling interrupts");
    asm!("msr DAIFSet, 0xf");
}
