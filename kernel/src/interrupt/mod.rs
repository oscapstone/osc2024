pub mod timer;

mod system_call;

use core::arch::global_asm;

use crate::process::{IOType, ProcessState, Registers};
use crate::{print, print_polling, println, println_polling};

use alloc::string::ToString;
use driver::mailbox;
use driver::uart;

use crate::fs::utils::path_parser;

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

pub fn c_str_to_str(c_str: *const u8) -> &'static str {
    let mut len = 0;
    while unsafe { *c_str.add(len) } != 0 {
        len += 1;
    }
    let c_str_slice = unsafe { core::slice::from_raw_parts(c_str, len) };
    let str = core::str::from_utf8(c_str_slice).unwrap();
    str
}

fn do_file_syscall(system_call_number: u64, sp: *mut u64) {
    let process_scheduler = unsafe { crate::PROCESS_SCHEDULER.as_mut().unwrap() };
    let pid = process_scheduler.get_current_pid();
    let process = process_scheduler.get_process(pid).unwrap();
    let vfs = unsafe { crate::GLOBAL_VFS.as_mut().unwrap() };

    match system_call_number {
        11 => {
            // int open(const char *pathname, int flags);
            unsafe {
                let pathname = process.registers.x0 as *const u8;
                let mut pathname_len = 0;
                while *pathname.add(pathname_len) != 0 {
                    pathname_len += 1;
                }
                let pathname = core::slice::from_raw_parts(pathname, pathname_len);
                let pathname_str_relative = core::str::from_utf8(pathname).unwrap();
                let flags = process.registers.x1 as u32;
                let pathname_str = 
                    path_parser(&(process.current_dir.clone() + "/" + pathname_str_relative));
                // println_polling!("file_syscall open: /{}, flags: {}", pathname_str, flags);
                let file = vfs.open(&pathname_str, flags != 0);
                match file {
                    Ok(file) => {
                        let fd = process.open_files.len() as u64;
                        process.open_files.push(file);
                        // println_polling!("fd: {}", fd);
                        process.registers.x0 = fd;
                    }
                    Err(e) => {
                        println_polling!("open error: {}", e);
                        process.registers.x0 = -1i64 as u64;
                    }
                }
                // println_polling!("open done");
            }
        }
        12 => {
            // println_polling!("close");
        }
        13 => {
            // long write(int fd, const void *buf, unsigned long count);
            unsafe {
                let fd = process.registers.x0 as usize;
                let buf = process.registers.x1 as *const u8;
                let count = process.registers.x2 as usize;
                let buf = core::slice::from_raw_parts(buf, count).to_vec();
                
                let file = process.open_files.get_mut(fd).unwrap();
                let write_len = file.write(&buf);
                // println_polling!("file_syscall: write");
                match write_len {
                    Ok(write_len) => {
                        process.registers.x0 = write_len as u64;
                    }
                    Err(e) => {
                        println_polling!("write error: {}", e);
                        process.registers.x0 = -1i64 as u64;
                    }
                }
            }
        }
        14 => {
            // long read(int fd, void *buf, unsigned long count);
            unsafe {
                let fd = process.registers.x0 as usize;
                let buf = process.registers.x1 as *mut u8;
                let count = process.registers.x2 as usize;
                // println_polling!("file_syscall: read fd {}", fd);
                let file = process.open_files.get_mut(fd);
                if file.is_none() {
                    process.registers.x0 = -1i64 as u64;
                    return;
                }
                let file = file.unwrap();
                let read_data = file.read(count);
                match read_data {
                    Ok(read_data) => {
                        unsafe {
                            core::ptr::copy(read_data.as_ptr(), buf, read_data.len());
                        }
                        process.registers.x0 = read_data.len() as u64;
                    }
                    Err(e) => {
                        println_polling!("read error: {}", e);
                        process.registers.x0 = -1i64 as u64;
                    }
                }
            }
        }
        15 => {
            // you can ignore mode, since there is no access control
            // int mkdir(const char *pathname, unsigned mode);
            // println_polling!("file_syscall: mkdir");
            unsafe {
                let pathname = c_str_to_str(process.registers.x0 as *const u8);
                let pathname = process.current_dir.clone() + "/" + path_parser(pathname).as_str();
                // println_polling!("mkdir: {}", pathname);
                let result = vfs.mkdir(&pathname);
                match result {
                    Ok(_) => {
                        process.registers.x0 = 0;
                    }
                    Err(e) => {
                        println_polling!("mkdir error: {}", e);
                        process.registers.x0 = -1i64 as u64;
                    }
                }
            }
        }
        16 => {
            // you can ignore arguments other than target (where to mount) and filesystem (fs name)
            // int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
            
            unsafe {
                let target = c_str_to_str(process.registers.x1 as *const u8);
                let filesystem = c_str_to_str(process.registers.x2 as *const u8);
                // println_polling!("mount: {} {}", target, filesystem);
                let fss = unsafe { &crate::FSS };
                // println_polling!("file_syscall: mount {} {}", target, filesystem);
                let fs = fss.iter().find(|f| {
                    let f = unsafe { f.as_ref() };
                    f.get_name() == filesystem
                });
                if fs.is_none() {
                    println_polling!("mount error: fs {} not found", filesystem);
                    process.registers.x0 = -1i64 as u64;
                    return;
                }
                let result = vfs.mount(*fs.unwrap(), target);
                match result {
                    Ok(_) => {
                        process.registers.x0 = 0;
                    }
                    Err(e) => {
                        println_polling!("mount error: {}", e);
                        process.registers.x0 = -1i64 as u64;
                    }
                }
            }
        }
        17 => {
            // int chdir(const char *path);
            unsafe {
                let path = c_str_to_str(process.registers.x0 as *const u8);
                // println_polling!("chdir: {}", path);
                process.current_dir = path_parser(path);
                process.registers.x0 = 0;
            }
        }
        18 => {
            // you only need to implement seek set
            // # define SEEK_SET 0
            // long lseek64(int fd, long offset, int whence);
            unsafe {
                let fd = process.registers.x0 as usize;
                let offset = process.registers.x1 as usize;
                let whence = process.registers.x2 as i64;
                let file = process.open_files.get_mut(fd).unwrap();
                let result = file.seek(offset, whence);
                match result {
                    Ok(offset) => {
                        process.registers.x0 = offset as u64;
                    }
                    Err(e) => {
                        println_polling!("lseek64 error: {}", e);
                        process.registers.x0 = -1i64 as u64;
                    }
                }
            }

        }
        _ => {
            panic!("沒看過，加油")
        }
    }
}

fn do_syscall(ec: u64, sp: *mut u64) {
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
                        let cpio_handler = crate::CPIO_HANDLER.as_mut().unwrap();
                        let mut file = cpio_handler
                            .get_files()
                            .find(|file| file.get_name() == prog_name_str)
                            .unwrap();
                        let file_size = file.get_size();
                        let file_data = file.read(file_size);
                        let process = process_scheduler
                            .get_process(process_scheduler.get_current_pid())
                            .unwrap();
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
                    process_scheduler.save_current_process(ProcessState::Ready, sp);
                    do_file_syscall(system_call_number, sp);
                    let next_pid =
                        process_scheduler.get_next_ready(process_scheduler.get_current_pid());

                    // run the next process
                    process_scheduler.set_next_process(next_pid);
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
    do_syscall(ec, sp);
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
    let irq_basic_pending: u32;
    const IRQ_BASE: *mut u64 = 0x3F00B000 as *mut u64;
    const IRQ_BASIC_PENDING: *mut u32 = 0x3F00B200 as *mut u32;
    const IRQ_PENDING_1: *mut u32 = 0x3F00B204 as *mut u32;

    let irq_pending_1: u32;
    let irq_source: u32;
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
                if let Some(current_process) =
                    process_scheduler.get_waiting_process(IOType::UartRead)
                {
                    current_process.do_uart_read();
                }
            }
        }
    }
}
