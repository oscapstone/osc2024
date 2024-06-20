use super::page::page_fault;
use crate::{exception::trap_frame, mmu::vm::VirtualMemory};
use core::{arch::asm, fmt::Debug};
use stdio::{debug, println};

#[repr(C)]
#[derive(Clone, Copy)]
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

impl Debug for Syscall {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(
            f,
            "Syscall {{ idx: 0x{:x}, arg0: 0x{:x}, arg1: 0x{:x}, arg2: 0x{:x}, arg3: 0x{:x}, arg4: 0x{:x}, arg5: 0x{:x}, arg6: 0x{:x}, arg7: 0x{:x} }}",
            self.idx, self.arg0, self.arg1, self.arg2, self.arg3, self.arg4, self.arg5, self.arg6, self.arg7
        )
    }
}

#[no_mangle]
unsafe fn lower_exception_handler(eidx: u64, sp: u64) {
    let esr_el1: u64;
    asm!(
        "mrs {0}, esr_el1",
        out(reg) esr_el1,
    );
    let ec = esr_el1 >> 26;
    match ec {
        0b010101 => svc_handler(sp),
        0b001110 => {
            panic!("Illegal Execution state.");
        }
        0b100000 | 0b100001 | 0b100100 => page_fault(),
        _ => {
            println!("Exception {}", eidx);
            println!("Unknown exception");
            println!("ec: 0b{:06b}", ec);
            el1_interrupt(sp);
        }
    }
}

unsafe fn svc_handler(sp: u64) {
    trap_frame::TRAP_FRAME = Some(trap_frame::TrapFrame::new(sp));
    syscall_handler(sp);
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
    let vm = VirtualMemory::load(trap_frame::TRAP_FRAME.as_ref().unwrap().state.l0);
    match syscall.idx {
        0 => {
            // println!("Syscall get_pid");
            let pid = crate::syscall::get_pid();
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = pid;
        }
        1 => {
            // println!("Syscall read");
            let buf = syscall.arg0 as *mut u8;
            let buf = vm.get_phys(buf as u64);
            let size = syscall.arg1 as usize;
            let read = crate::syscall::read(buf, size);
            assert!(size == 1);
            if read == 0 {
                trap_frame::TRAP_FRAME.as_mut().unwrap().state.pc -= 4;
            } else {
                trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = read as u64;
            }
        }
        2 => {
            // println!("Syscall write");
            let buf = syscall.arg0 as *const u8;
            let buf = vm.get_phys(buf as u64);
            let size = syscall.arg1 as usize;
            let written = crate::syscall::write(buf, size);
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = written as u64;
        }
        3 => {
            // println!("Syscall exec");
            let name = syscall.arg0 as *const u8;
            let name = vm.get_phys(name as u64);
            let ret = crate::syscall::exec(name);
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = ret;
        }
        4 => {
            // println!("Syscall fork");
            let pid = crate::syscall::fork();
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = pid;
        }
        5 => {
            // println!("Syscall exit");
            crate::syscall::exit(syscall.arg0);
        }
        6 => {
            // println!("Syscall mbox_call");
            let channel = syscall.arg0 as u8;
            let mbox = syscall.arg1 as *mut u32;
            let mbox = vm.get_phys(mbox as u64) as *mut u32;
            let ret = crate::syscall::mbox_call(channel, mbox);
            trap_frame::TRAP_FRAME.as_mut().unwrap().state.x[0] = ret as u64;
        }
        7 => {
            // println!("Syscall kill");
            let pid = syscall.arg0;
            crate::syscall::kill(pid);
        }
        _ => {
            println!("Unknown syscall: 0x{:x}", syscall.idx);
            panic!("Unknown syscall");
        }
    }
}
