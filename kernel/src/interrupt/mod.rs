use crate::println;
use driver::uart;

use core::ptr::read_volatile;

#[no_mangle]
#[inline(never)]
pub fn rust_frq_handler() {
    println!("FRQ interrupt!");
}

#[no_mangle]
#[inline(never)]
pub fn rust_sync_handler() {
    let spsr_el1: u64;
    let elr_el1: u64;
    let esr_el1: u64;
    unsafe {
        core::arch::asm!("mrs {0}, spsr_el1", out(reg) spsr_el1);
        core::arch::asm!("mrs {0}, elr_el1", out(reg) elr_el1);
        core::arch::asm!("mrs {0}, esr_el1", out(reg) esr_el1);
    }
    println!("spsr_el1: {:#x}", spsr_el1);
    println!("elr_el1: {:#x}", elr_el1);
    println!("esr_el1: {:#x}", esr_el1);
}

#[no_mangle]
#[inline(never)]
pub fn rust_core_timer_handler() {
    // println!("Core timer interrupt!");
    // // read cntpct_el0 and cntfrq_el0
    // let mut cntpct_el0: u64;
    // let mut cntfrq_el0: u64;
    // unsafe {
    //     core::arch::asm!("mrs {0}, cntpct_el0", out(reg) cntpct_el0);
    //     core::arch::asm!("mrs {0}, cntfrq_el0", out(reg) cntfrq_el0);
    //     core::arch::asm!("msr cntp_tval_el0, {0}", in(reg) cntfrq_el0 << 1);
    // }
    // let mut seconds = cntpct_el0 / cntfrq_el0;
    // println!("The seconds since boot: {}s", seconds);
    crate::timer::timer_handler();
}

#[no_mangle]
fn rust_serr_handler() {
    // print the content of spsr_el1, elr_el1, and esr_el1
    println!("SErr happened!");
}

#[no_mangle]
fn rust_irq_handler() {
    // read IRQ basic pending register
    let mut irq_basic_pending: u32;
    const IRQ_BASE: *mut u64 = 0x3F00B000 as *mut u64;
    const IRQ_BASIC_PENDING: *mut u32 = 0x3F00B200 as *mut u32;
    const IRQ_PENDING_1: *mut u32 = 0x3F00B204 as *mut u32;
    
    let mut irq_pending_1: u32;
    let mut irq_source: u32;
    const CORE0_IRQ_SOURCE: u64 = 0x40000060;
    unsafe {
        irq_pending_1 = read_volatile(IRQ_PENDING_1 as *mut u32);
        irq_source = read_volatile(CORE0_IRQ_SOURCE as *mut u32);
        irq_basic_pending = read_volatile(IRQ_BASIC_PENDING);
    }
    // println!("IRQ basic pending register: {:#b}", irq_basic_pending);

    if irq_source & 0b10 != 0 {
        rust_core_timer_handler();
    }
    // there is a pending interrupt in the pending register 1
    if irq_basic_pending & (0b1 << 8) != 0 {
        // interrupt from GPU
        let iir = unsafe { read_volatile(uart::AUX_MU_IIR_REG as *mut u32) };
        if iir & 0b10 != 0 {
            // Transmit holding register empty
            uart::send_write_buf();
        } else if iir & 0b100 != 0 {
            if let Some(u) = unsafe { uart::read_u8() } {
                uart::push_read_buf(u);
            };
        }
    }
}

