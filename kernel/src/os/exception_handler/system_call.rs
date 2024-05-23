use super::super::thread;
use super::trap_frame;
use crate::cpu::uart;
use alloc::{string::String, vec::Vec};
use core::ptr::{read_volatile, write_volatile};

pub unsafe fn get_pid(trap_frame_ptr: *mut u64, current_pc: u64) {
    let pid = thread::get_id_by_pc(current_pc as usize).expect("Failed to get PID");
    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, pid as u64);
}

pub unsafe fn read_from_uart(trap_frame_ptr: *mut u64) {
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
            }
        }
    }

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, idx as u64);
}

pub unsafe fn write_to_uart(trap_frame_ptr: *mut u64) {
    let buf = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *const u8;
    let size = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as usize;

    for i in 0..size {
        uart::send_async(read_volatile(buf.add(i)));
    }

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, size as u64);
}

pub unsafe fn exec(trap_frame_ptr: *mut u64) {
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
    thread::restore_context(trap_frame_ptr);
}

pub unsafe fn fork(trap_frame_ptr: *mut u64) {
    thread::save_context(trap_frame_ptr);
    match thread::fork() {
        Some(pid) => trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, pid as u64),
        None => panic!("Fork failed"),
    }
}

pub unsafe fn exit(trap_frame_ptr: *mut u64) {
    let pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC) as usize;
    let pid = thread::get_id_by_pc(pc).expect("Failed to get PID");
    thread::kill(pid);
    thread::switch_to_thread(None, trap_frame_ptr);
}

pub unsafe fn mailbox_call(trap_frame_ptr: *mut u64) {
    // mail box
    let channel = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as u8;
    let mbox_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *mut u32;

    crate::cpu::mailbox::mailbox_call(channel, mbox_ptr);

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, 1);
}

pub unsafe fn kill(trap_frame_ptr: *mut u64) {
    let pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC) as usize;
    let my_pid = thread::get_id_by_pc(pc).expect("Failed to get PID");
    let pid = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as usize;
    if my_pid != pid {
        thread::kill(pid);
    } else {
        println!("Cannot kill itself");
    }
}
