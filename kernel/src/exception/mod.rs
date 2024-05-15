mod handlers;
pub mod trap_frame;

use core::arch::{asm, global_asm};
use driver::mmio::Mmio;
use stdio::debug;

global_asm!(include_str!("context_switch.S"));
global_asm!(include_str!("exception_table.S"));

#[no_mangle]
unsafe fn unknown_exception_handler(eidx: u64) {
    let esr_el1: u64;
    let elr_el1: u64;
    asm!(
        "mrs {0}, esr_el1", out(reg) esr_el1,
    );
    asm!(
        "mrs {0}, elr_el1", out(reg) elr_el1,
    );
    debug!("ESR_EL1: 0x{:x}", esr_el1);
    debug!("ELR_EL1: 0x{:x}", elr_el1);
    panic!("Unknown exception {}", eidx);
}
