use core::{
    arch::{asm, global_asm},
    ptr::read_volatile,
};
use driver::mmio::{regs::AuxReg, regs::MmioReg, Mmio};
use stdio::{debug, println};

global_asm!(include_str!("context_switch.S"));

#[no_mangle]
unsafe fn exception_handler(idx: u64) {
    disable_interrupt();
    match idx {
        0x4 => {
            let esr_el1: u64;
            asm!(
                "mrs {0}, esr_el1", out(reg) esr_el1,
            );
            debug!("Exception {}", idx);
            debug!("ESR_EL1: 0x{:x}", esr_el1);
            let elr_el1: u64;
            asm!(
                "mrs {0}, elr_el1", out(reg) elr_el1,
            );
            debug!("ELR_EL1: 0x{:x}", elr_el1);

            for _ in 0..1000000 {
                asm!("nop");
            }
        }
        0x5 => {
            if read_volatile(0x4000_0060 as *const u32) == 0x02 {
                {
                    let tm = crate::timer::manager::get();
                    tm.handle_interrupt();
                }
            }
            if Mmio::read_reg(MmioReg::Aux(AuxReg::Irq)) & 0x1 == 0x1 {
                // println!("irq interrupt");
                driver::uart::handle_irq();
            }
        }
        0x8 => {
            println!("SVC call");
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
        }
        _ => {
            debug!("Exception {}", idx);
            debug!("Unknown exception");
            for _ in 0..1000000 {
                asm!("nop");
            }
        }
    }
    enable_interrupt();
}

#[allow(dead_code)]
#[inline(always)]
pub unsafe fn enable_interrupt() {
    asm!("msr DAIFClr, 0xf");
}

#[allow(dead_code)]
#[inline(always)]
pub unsafe fn disable_interrupt() {
    asm!("msr DAIFSet, 0xf");
}
