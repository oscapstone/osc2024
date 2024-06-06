pub mod timer;

mod system_call;

use core::arch::global_asm;

use crate::fs::cpio;
use crate::process::{self, IOType, ProcessScheduler, ProcessState, Registers};
use crate::{print, print_polling, println, println_polling};

use driver::mailbox;
use driver::uart;

use core::{panic, ptr::read_volatile};

global_asm!(include_str!("interrupt.s"));

#[no_mangle]
#[inline(never)]
pub fn rust_frq_handler() {
    println!("FRQ interrupt!");
}

#[no_mangle]
#[inline(never)]
pub fn int_boink() {
    unsafe { core::arch::asm!("nop") };
}

#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_sync_handler(sp: *mut u64) {
    // check the interrupt source
    let esr_el1: u64;
    let spsr_el1: u64;
    let elr_el1: u64;
    unsafe {
        core::arch::asm!("mrs {0}, spsr_el1", out(reg) spsr_el1);
        core::arch::asm!("mrs {0}, elr_el1", out(reg) elr_el1);
        core::arch::asm!("mrs {0}, esr_el1", out(reg) esr_el1);
    }
    // check the interrupt source
    let ec = (esr_el1 >> 26) & 0b111111;
    let iss = esr_el1 & 0xFFFFFF;
    match ec {
        0b010101 => {
            // println_polling!("svc call from user program");
            // check the system call number
            let process_scheduler = unsafe { crate::PROCESS_SCHEDULER.as_mut().unwrap() };
            process_scheduler.ksp = sp;
            let mut registers: *mut Registers = sp as *mut Registers;
            let system_call_number = unsafe { (*registers).x8 };
            match system_call_number {
                0 => {
                    // get pid
                    println_polling!("get pid");
                    let current_pid = process_scheduler.get_current_pid();
                    unsafe {
                        (*registers).x0 = current_pid as u64;
                    }
                }
                1 => {
                    // uart read
                    // println_polling!("uart read");
                    process_scheduler
                        .save_current_process(ProcessState::Waiting(IOType::UartRead), sp);
                    let current_pid = process_scheduler.get_current_pid();
                    // println_polling!("process: {} is waiting for uart read", current_pid);
                    process_scheduler.next_process();
                }
                2 => {
                    // uart write
                    // println_polling!("uart write");
                    unsafe {
                        let buf: *mut u8 = (*registers).x0 as *mut u8;
                        let buf_len: usize = (*registers).x1 as usize;
                        let write_array = core::slice::from_raw_parts(buf, buf_len);
                        for i in write_array {
                            uart::write_u8(*i);
                        }
                    }
                }
                3 => {
                    // exec
                    unsafe {
                        let prog_name: *const u8 = (*registers).x0 as *const u8;
                        // from c string to rust string
                        let mut prog_name_len = 0;
                        while *prog_name.add(prog_name_len) != 0 {
                            prog_name_len += 1;
                        }
                        let prog_name_array = core::slice::from_raw_parts(prog_name, prog_name_len);
                        let prog_name_str = core::str::from_utf8(prog_name_array).unwrap();
                        println_polling!("exec: {}", prog_name_str);
                        let cpio_handler = unsafe { crate::CPIO_HANDLER.as_mut().unwrap() };
                        let mut file = cpio_handler.get_files().find(|file| file.get_name() == prog_name_str).unwrap();
                        let file_size = file.get_size();
                        let file_data = file.read(file_size);
                        let process = process_scheduler.get_process(process_scheduler.get_current_pid()).unwrap();
                        process.exec(file_data);
                        process_scheduler.next_process();
                    }
                }
                4 => {
                    // fork
                    int_boink();
                    process_scheduler.save_current_process(ProcessState::Running, sp);

                    let new_pid = process_scheduler.fork();
                    
                    // let current process is the parent process
                    let current_pid = process_scheduler.get_current_pid();
                    let mut parent_process = process_scheduler.get_process(current_pid).unwrap();
                    parent_process.registers.x0 = new_pid as u64;
                    println_polling!("fork: processes {:?}", process_scheduler.processes);
                    unsafe {
                        (*registers).x0 = new_pid as u64;
                    }
                }
                5 => {
                    // exit
                    process_scheduler.terminate(process_scheduler.get_current_pid());
                }
                6 => {
                    // mbox call
                    let ch = unsafe { (*registers).x0 as u32 };
                    let mailbox = unsafe { (*registers).x1 as *const u32 };
                    // print_polling!("mbox call: ch: {}, mailbox: {:?}", ch, mailbox);
                    // println_polling!("mbox call: ch: {}, mailbox: {:?}", ch, mailbox);
                    mailbox::mailbox_call(ch, mailbox);
                    unsafe {
                        (*registers).x0 = 1 as u64;
                    }
                }
                7 => {
                    // kill
                    let pid = unsafe { (*registers).x0 as usize };
                    process_scheduler.terminate(pid);
                }
                _ => {
                    panic!("沒看過，加油")
                }
            }
        }
        _ => {
            panic!("Unknown synchronous exception");
        }
    }
}

#[no_mangle]
#[inline(never)]
pub fn rust_core_timer_handler(sp: *mut u64) {
    crate::timer::timer_handler(sp);
}

#[no_mangle]
fn rust_serr_handler() {
    // print the content of spsr_el1, elr_el1, and esr_el1
    println!("SErr happened!");
}

#[no_mangle]
fn rust_irq_handler(sp: *mut u64) {
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
        rust_core_timer_handler(sp);
    }
    // there is a pending interrupt in the pending register 1
    if irq_basic_pending & (0b1 << 8) != 0 {
        // interrupt from GPU
        let process_scheduler = unsafe { crate::PROCESS_SCHEDULER.as_mut().unwrap() };

        let iir = unsafe { read_volatile(uart::AUX_MU_IIR_REG as *mut u32) };
        if iir & 0b10 != 0 {
            // Transmit holding register empty
            uart::send_write_buf();
        } else if iir & 0b100 != 0 {
            if let Some(u) = unsafe { uart::read_u8() } {
                uart::push_read_buf(u);
            };
            if process_scheduler.is_running() {
                if let Some(mut current_process) =
                    process_scheduler.get_waiting_process(IOType::UartRead)
                {
                    current_process.do_uart_read();
                }
            }
        }
    }
}
