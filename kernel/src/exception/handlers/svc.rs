use crate::exception::trap_frame;
use core::arch::asm;
use stdio::{debug, println};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
struct Syscall {
    arg0: u64,
    arg1: u64,
    arg2: u64,
    arg3: u64,
    arg4: u64,
    arg5: u64,
    arg6: u64,
    arg7: u64,
    idx: u64,
}

use core::ptr::read_volatile;
impl Syscall {
    pub fn new(sp: u64) -> Self {
        let arg0 = unsafe { read_volatile(sp as *const u64) };
        let arg1 = unsafe { read_volatile((sp + 8) as *const u64) };
        let arg2 = unsafe { read_volatile((sp + 16) as *const u64) };
        let arg3 = unsafe { read_volatile((sp + 24) as *const u64) };
        let arg4 = unsafe { read_volatile((sp + 32) as *const u64) };
        let arg5 = unsafe { read_volatile((sp + 40) as *const u64) };
        let arg6 = unsafe { read_volatile((sp + 48) as *const u64) };
        let arg7 = unsafe { read_volatile((sp + 56) as *const u64) };
        let idx = unsafe { read_volatile((sp + 64) as *const u64) };
        Self {
            arg0,
            arg1,
            arg2,
            arg3,
            arg4,
            arg5,
            arg6,
            arg7,
            idx,
        }
    }
}

#[no_mangle]
unsafe fn svc_handler(eidx: u64, sp: u64) {
    trap_frame::TRAP_FRAME = Some(trap_frame::TrapFrame::new(sp));
    match eidx {
        4 => el1_interrupt(sp),
        8 => syscall_handler(sp),
        _ => {
            println!("Exception {}", eidx);
            println!("Unknown exception");
        }
    }
    trap_frame::TRAP_FRAME.unwrap().restore();
    trap_frame::TRAP_FRAME = None;
}

unsafe fn el1_interrupt(sp: u64) {
    let current_el: u64;
    asm!(
        "mrs {0}, CurrentEL", out(reg) current_el,
    );
    debug!("CurrentEL: 0x{:x}", current_el);
    let esr_el1: u64;
    asm!(
        "mrs {0}, esr_el1", out(reg) esr_el1,
    );
    debug!("ESR_EL1: 0x{:x}", esr_el1);
    let elr_el1: u64;
    asm!(
        "mrs {0}, elr_el1", out(reg) elr_el1,
    );
    debug!("ELR_EL1: 0x{:x}", elr_el1);
    if esr_el1 == 0x5600_0000 {
        let syscall = Syscall::new(sp);
        debug!("Syscall idx: {}", syscall.idx);
        println!("syscall: {:?}", syscall);
    }
    panic!("Segmentation fault");
}

unsafe fn syscall_handler(sp: u64) {
    let syscall = Syscall::new(sp);
    assert!(trap_frame::TRAP_FRAME.is_some());
    match syscall.idx {
        0 => {
            // println!("Syscall get_pid");
            let pid = crate::syscall::get_pid();
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = pid;
        }
        1 => {
            // println!("Syscall read");
            let buf = syscall.arg0 as *mut u8;
            let size = syscall.arg1 as usize;
            let read = crate::syscall::read(buf, size);
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = read as u64;
        }
        2 => {
            // println!("Syscall write");
            let buf = syscall.arg0 as *const u8;
            let size = syscall.arg1 as usize;
            let written = crate::syscall::write(buf, size);
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = written as u64;
        }
        5 => {
            // println!("Syscall exit");
            crate::syscall::exit(syscall.arg0);
        }
        _ => {
            println!("Unknown syscall: 0x{:x}", syscall.idx);
            panic!("Unknown syscall");
        }
    }
}
