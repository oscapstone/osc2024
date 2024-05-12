use crate::os::stdio;

use super::{
    super::{cpu::uart, os::timer},
    stdio::{print_hex_now, println_now},
};
use core::{
    arch::{asm, global_asm},
    ptr::{read_volatile, write_volatile},
};
mod trap_frame;

global_asm!(include_str!("context_switching.s"));

#[no_mangle]
unsafe extern "C" fn irq_handler_rust(trap_frame_ptr: *mut u64) {
    let interrupt_source = read_volatile(0x40000060 as *const u32);

    // let spsr_el1: u64;
    // let elr_el1: u64;
    // let esr_el1: u64;
    // asm!("mrs {spsr_el1}, spsr_el1", spsr_el1 = out(reg) spsr_el1);
    // asm!("mrs {elr_el1}, elr_el1", elr_el1 = out(reg) elr_el1);
    // asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);

    if interrupt_source & 0x2 > 0 {
        timer::irq_handler();
    }

    if read_volatile(0x3F21_5000 as *const u32) & 0x1 == 0x1 {
        uart::irq_handler();
    }

    // asm!("msr spsr_el1, {spsr_el1}", spsr_el1 = in(reg) spsr_el1);
}

#[no_mangle]
unsafe extern "C" fn svc_handler_rust(trap_frame_ptr: *mut u64) {
    let esr_el1: u64;
    asm!("mrs {esr_el1}, esr_el1", esr_el1 = out(reg) esr_el1);

    // println!("ESR_EL1: {:x}", esr_el1);

    if esr_el1 == 0x56000000 {
        let system_call_num = trap_frame::get(trap_frame_ptr, trap_frame::TrapFrameRegister::X8);
        // println!("System call number: {}", system_call_num);
        match system_call_num {
            1 => {
                let buf =
                    trap_frame::get(trap_frame_ptr, trap_frame::TrapFrameRegister::X0) as *mut u8;
                let size =
                    trap_frame::get(trap_frame_ptr, trap_frame::TrapFrameRegister::X1) as usize;

                let mut idx = 0;
                while idx < size {
                    match uart::recv_async() {
                        Some(byte) => write_volatile(buf.add(idx), byte),
                        None => break,
                    }
                    idx += 1;
                }

                trap_frame::set(
                    trap_frame_ptr,
                    trap_frame::TrapFrameRegister::X0,
                    idx as u64,
                );
            }
            2 => {
                let buf =
                    trap_frame::get(trap_frame_ptr, trap_frame::TrapFrameRegister::X0) as *const u8;
                let size =
                    trap_frame::get(trap_frame_ptr, trap_frame::TrapFrameRegister::X1) as usize;

                for i in 0..size {
                    uart::send_async(read_volatile(buf.add(i)));
                }

                trap_frame::set(
                    trap_frame_ptr,
                    trap_frame::TrapFrameRegister::X0,
                    size as u64,
                );
            }
            _ => (),
        }
    }
}
