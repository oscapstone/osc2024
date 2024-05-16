use crate::exception::trap_frame;
use crate::exception::Mmio;
use core::ptr::read_volatile;
use driver::mmio::regs::AuxReg;
use driver::mmio::regs::MmioReg;

#[no_mangle]
unsafe fn irq_handler(eidx: u64, sp: u64) {
    trap_frame::TRAP_FRAME = Some(trap_frame::TrapFrame::new(sp));
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
                driver::uart::handle_irq();
            }
        }
        _ => {
            panic!("Unknown interrupt")
        }
    }

    if let Some(tf) = trap_frame::TRAP_FRAME.as_ref() {
        tf.restore();
    }
    trap_frame::TRAP_FRAME = None;
}
