use crate::exception::disable_interrupt;
use crate::exception::enable_interrupt;
use crate::exception::Mmio;
use core::arch::asm;
use core::ptr::read_volatile;
use driver::mmio::regs::AuxReg;
use driver::mmio::regs::MmioReg;
use stdio::debug;
use stdio::println;

#[no_mangle]
unsafe fn irq_handler(idx: u64) {
    disable_interrupt();
    match idx {
        0x5 => {
            if read_volatile(0x4000_0060 as *const u32) == 0x02 {
                {
                    let tm = crate::timer::manager::get();
                    debug!("Timer interrupt at {:#?}", tm.current_time());
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
            let mut syscall: u32;
            asm!("mov {0}, x8", out(reg) syscall, lateout("x8") _);
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

            match syscall {
                0 => {}
                _ => {
                    println!("Unknown syscall: 0x{:x}", syscall);
                }
            }
            loop {}
        }
        _ => {
            panic!("Unknown interrupt")
        }
    }
    enable_interrupt();
}
