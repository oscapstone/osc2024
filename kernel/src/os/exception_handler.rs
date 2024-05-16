use alloc::{
    string::String,
    vec::Vec,
};

use crate::os::{
    file_system::cpio, stdio::{self, println}, thread::{self, context_switching, get_id_by_pc, restore_context}
};

use super::{
    super::{cpu::uart, os::timer},
    stdio::{print_hex_now, println_now},
};
use core::{
    arch::{asm, global_asm},
    ptr::{read_volatile, write_volatile},
};
pub mod trap_frame;

global_asm!(include_str!("context_switching.s"));

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
        // println!("System call number: {}", system_call_num);
        match system_call_num {
            0 => {
                // Get PID
                let pid = thread::get_id_by_pc(elr_el1 as usize).unwrap();

                trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, pid as u64);
            }
            1 => {
                // Read from UART
                let buf = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;
                let size = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as usize;

                let mut idx = 0;
                while idx < size {
                    match uart::recv_async() {
                        Some(byte) => {
                            write_volatile(buf.add(idx), byte);
                            idx += 1;
                        }
                        None => {
                            if idx == 0 {
                                let pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC);
                                trap_frame::set(trap_frame_ptr, trap_frame::Register::PC, pc - 4);
                                thread::context_switching();
                            }
                            break;
                        },
                    }
                    // write_volatile(buf.add(idx), uart::recv());
                    // idx += 1;
                }

                trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, idx as u64);
            }
            2 => {
                // Write to UART
                let buf = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *const u8;
                let size = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as usize;

                for i in 0..size {
                    uart::send_async(read_volatile(buf.add(i)));
                }

                trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, size as u64);
            }
            3 => {
                // exec
                let mut program_name_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;
                let program_args_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *mut u8;

                let mut program_name = String::new();
                loop {
                    let byte = read_volatile(program_name_ptr);
                    if byte == 0 {
                        break;
                    }
                    program_name.push(byte as char);
                    program_name_ptr = program_name_ptr.add(1);
                }

                thread::exec(program_name, Vec::new());
                restore_context(trap_frame_ptr);
            }
            4 => {
                // fork
                thread::save_context(trap_frame_ptr);
                match thread::fork() {
                    Some(pid) => trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, pid as u64),
                    None => panic!("Fork failed"),
                }
                println_now("Forked");
            }
            5 => {
                // exit
                let pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC) as usize;
                let pid = get_id_by_pc(pc).expect("Failed to get PID");
                thread::kill(pid);
                thread::switch_to_thread(None, trap_frame_ptr);
            }
            6 => {
                // mail box
                let channel = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as u8;
                let mbox_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *mut u32;

                crate::cpu::mailbox::mailbox_call(channel, mbox_ptr);
                
                trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, 1);
            }
            7 => {
                // kill
                let pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC) as usize;
                let my_pid = get_id_by_pc(pc).expect("Failed to get PID");
                let pid = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as usize;
                if my_pid != pid {
                    thread::kill(pid);
                } else {
                    println!("Cannot kill itself");
                }
                // thread::switch_to_thread(None, trap_frame_ptr);
            }
            _ => {
                println_now("Unknown system call number: ");
                print_hex_now(system_call_num as u32);
                loop{}
                panic!("Unknown system call number: {}", system_call_num);
            },
        }
    } else {
        println!("Unknown exception: {:08x}", esr_el1);
        println!("ELR_EL1: {:08x}", elr_el1);
        println!("SP_EL0: {:08x}", sp_el0);
        loop {}
    }

    thread::TRAP_FRAME_PTR = None;
}
