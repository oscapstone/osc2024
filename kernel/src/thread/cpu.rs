use core::fmt::{self, Debug};
use core::ptr::read_volatile;
use core::ptr::write_volatile;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct State {
    pub x: [u64; 31],
    pub pc: u64,
    pub sp: u64,
    spsr: u64,
    pub l0: u64,
}

impl State {
    pub fn new(stack: *mut u8, stack_size: usize, entry: *mut u8, vm: *mut u8) -> Self {
        let sp = stack as u64 + stack_size as u64;
        let x = [0; 31];
        // x[30] = entry as u64;
        let spsr = 0x0;
        State {
            x,
            pc: entry as u64,
            sp,
            spsr,
            l0: vm as u64,
        }
    }

    pub fn load(addr: u64) -> Self {
        let mut x = [0; 31];
        for i in 0..31 {
            x[i] = unsafe { read_volatile((addr + i as u64 * 8) as *const u64) };
        }
        let pc = unsafe { read_volatile((addr + 32 * 8) as *const u64) };
        let sp = unsafe { read_volatile((addr + 33 * 8) as *const u64) };
        let spsr = unsafe { read_volatile((addr + 34 * 8) as *const u64) };
        let l0 = unsafe { read_volatile((addr + 35 * 8) as *const u64) };
        // println!("spsr: 0x{:x}", spsr);
        // assert!((spsr & 0xfff) == 0x0, "SPSR is not zero 0x{:x}", spsr);
        // spsr = 0x0;
        State {
            x,
            pc,
            sp,
            spsr,
            l0,
        }
    }

    pub fn store(&self, addr: u64) {
        for i in 0..31 {
            unsafe { write_volatile((addr + i as u64 * 8) as *mut u64, self.x[i]) };
        }
        unsafe { write_volatile((addr + 32 * 8) as *mut u64, self.pc) };
        unsafe { write_volatile((addr + 33 * 8) as *mut u64, self.sp) };
        unsafe { write_volatile((addr + 34 * 8) as *mut u64, self.spsr) };
        unsafe { write_volatile((addr + 35 * 8) as *mut u64, self.l0) };
    }
}

impl Debug for State {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "State {{ x: [")?;
        for i in 0..31 {
            write!(f, "0x{:x}, ", self.x[i])?;
        }
        write!(
            f,
            "], pc: 0x{:x}, sp: 0x{:x}, spsr: 0x{:x}, l0: 0x{:x} }}",
            self.pc, self.sp, self.spsr, self.l0
        )
    }
}
