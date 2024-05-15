use crate::exception::disable_interrupt;
use crate::exception::enable_interrupt;
use crate::exception::trap_frame;
use crate::exception::Mmio;
use core::ptr::read_volatile;
use driver::mmio::regs::AuxReg;
use driver::mmio::regs::MmioReg;
#[allow(unused_imports)]
use stdio::{debug, println};

#[no_mangle]
unsafe fn irq_handler(eidx: u64, sp: u64) {
    // disable_interrupt();
    trap_frame::TRAP_FRAME = Some(trap_frame::TrapFrame::new(sp));
    // println!("Trap frame: {:?}", trap_frame::TRAP_FRAME.as_ref().unwrap());
    match eidx {
        5 | 9 => {
            if read_volatile(0x4000_0060 as *const u32) == 0x02 {
                {
                    let tm = crate::timer::manager::get();
                    // debug!("Timer interrupt at {:#?}", tm.current_time());
                    tm.handle_interrupt();
                }
            }
            if Mmio::read_reg(MmioReg::Aux(AuxReg::Irq)) & 0x1 == 0x1 {
                debug!("UART interrupt");
                driver::uart::handle_irq();
                {
                    use core::arch::asm;
                    let esr_el1: u64;
                    let elr_el1: u64;
                    asm!(
                        "mrs {0}, esr_el1", out(reg) esr_el1,
                    );
                    asm!(
                        "mrs {0}, elr_el1", out(reg) elr_el1,
                    );
                    // debug!("ESR_EL1: 0x{:x}", esr_el1);
                    // debug!("ELR_EL1: 0x{:x}", elr_el1);
                }
            }
        }
        _ => {
            panic!("Unknown interrupt")
        }
    }
    // println!(
    //     "Restored trap frame: {:?}",
    //     trap_frame::TRAP_FRAME.as_ref().unwrap()
    // );
    if let Some(tf) = trap_frame::TRAP_FRAME.as_ref() {
        tf.restore();
    }
    trap_frame::TRAP_FRAME = None;
    // enable_interrupt();
}
