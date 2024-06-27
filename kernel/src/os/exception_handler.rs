use alloc::format;

use crate::cpu::uart;
use crate::os::stdio::{print_hex_now, println_now};
use crate::os::{thread, timer};

use core::{
    arch::{asm, global_asm},
    ptr::read_volatile,
};

mod system_call;
pub mod trap_frame;

global_asm!(include_str!("exception_handler/exception_handler.s"));

#[no_mangle]
unsafe extern "C" fn exception_handler_rust(trap_frame_ptr: *mut u64) {
    println_now("Unknown exception");
    panic!("Unknown exception");
}

#[no_mangle]
unsafe extern "C" fn irq_handler_rust(trap_frame_ptr: *mut u64) {
    thread::TRAP_FRAME_PTR = Some(trap_frame_ptr);

    let interrupt_source = read_volatile(0xFFFF_0000_4000_0060 as *const u32);

    if interrupt_source & 0x2 > 0 {
        timer::irq_handler();
    }

    if read_volatile(0xFFFF_0000_3F21_5000 as *const u32) & 0x1 == 0x1 {
        uart::irq_handler();
    }

    thread::TRAP_FRAME_PTR = None;
}

#[no_mangle]
unsafe extern "C" fn svc_handler_rust(trap_frame_ptr: *mut u64) {
    let esr_el1: u64;
    let elr_el1: u64;
    let sp_el0: u64;
    let sp_el1: u64;

    asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);
    asm!("mrs {elr_el1}, elr_el1", elr_el1 = out(reg) elr_el1);
    asm!("mrs {sp_el0}, sp_el0", sp_el0 = out(reg) sp_el0);
    asm!("mrs {sp_el1}, sp_el1", sp_el1 = out(reg) sp_el1);
    thread::TRAP_FRAME_PTR = Some(trap_frame_ptr);

    assert_eq!(elr_el1, trap_frame::get(trap_frame_ptr, trap_frame::Register::PC), "ELR_EL1 and PC mismatch");

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
            11 => system_call::open(trap_frame_ptr),
            12 => system_call::close(trap_frame_ptr),
            13 => system_call::write(trap_frame_ptr),
            14 => system_call::read(trap_frame_ptr),
            15 => system_call::mkdir(trap_frame_ptr),
            16 => system_call::mount(trap_frame_ptr),
            17 => system_call::chdir(trap_frame_ptr),
            _ => panic!("Unknown system call number: {}", system_call_num),
        }
        // println!("System call number: {}", system_call_num);
    } else {
        println_now("Unknown exception");
        println_now(format!("ESR_EL1: {:08x}", esr_el1).as_str());
        println_now(format!("ELR_EL1: {:08x}", elr_el1).as_str());
        println_now(format!("SP_EL0: {:08x}", sp_el0).as_str());
        println_now(format!("SP_EL1: {:08x}", sp_el1).as_str());
        panic!();
    }

    thread::TRAP_FRAME_PTR = None;
}
