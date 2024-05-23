use crate::cpu::uart;
use crate::os::{thread, timer};

use core::{
    arch::{asm, global_asm},
    ptr::read_volatile,
};

use super::stdio::println_now;
mod system_call;
pub mod trap_frame;

global_asm!(include_str!("exception_handler/exception_handler.s"));

#[no_mangle]
unsafe extern "C" fn irq_handler_rust(trap_frame_ptr: *mut u64) {
    thread::TRAP_FRAME_PTR = Some(trap_frame_ptr);

    let interrupt_source = read_volatile(0x40000060 as *const u32);

    if interrupt_source & 0x2 > 0 {
        timer::irq_handler();
    }

    if read_volatile(0x3F21_5000 as *const u32) & 0x1 == 0x1 {
        uart::irq_handler();
    }

    thread::TRAP_FRAME_PTR = None;
}

#[no_mangle]
unsafe extern "C" fn svc_handler_rust(trap_frame_ptr: *mut u64) {
    let esr_el1: u64;
    let elr_el1: u64;
    let sp_el0: u64;
    asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);
    asm!("mrs {elr_el1}, elr_el1", elr_el1 = out(reg) elr_el1);
    asm!("mrs {sp_el0}, sp_el0", sp_el0 = out(reg) sp_el0);
    thread::TRAP_FRAME_PTR = Some(trap_frame_ptr);

    // println!("ESR_EL1: {:08x}", esr_el1);
    // println!("ELR_EL1: {:08x}", elr_el1);
    // println!("SP_EL0: {:08x}", sp_el0);

    if esr_el1 == 0x56000000 {
        let system_call_num = trap_frame::get(trap_frame_ptr, trap_frame::Register::X8);
        match system_call_num {
            0 => system_call::get_pid(trap_frame_ptr, elr_el1),
            1 => system_call::read_from_uart(trap_frame_ptr),
            2 => system_call::write_to_uart(trap_frame_ptr),
            3 => system_call::exec(trap_frame_ptr),
            4 => system_call::fork(trap_frame_ptr),
            5 => system_call::exit(trap_frame_ptr),
            6 => system_call::mailbox_call(trap_frame_ptr),
            7 => system_call::kill(trap_frame_ptr),
            _ => panic!("Unknown system call number: {}", system_call_num),
        }
        // println!("System call number: {}", system_call_num);
    } else {
        println!("Unknown exception: {:08x}", esr_el1);
        println!("ELR_EL1: {:08x}", elr_el1);
        println!("SP_EL0: {:08x}", sp_el0);
        loop {}
    }

    thread::TRAP_FRAME_PTR = None;
}
