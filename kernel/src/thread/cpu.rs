use core::fmt::{self, Debug};
use core::ptr::read_volatile;
use core::ptr::write_volatile;
#[repr(C)]
#[derive(Clone, Copy)]
pub struct State {
    x: [u64; 31],
    pub pc: u64,
    sp: u64,
}

impl State {
    pub fn new(stack: *mut u8, stack_size: usize, entry: extern "C" fn()) -> Self {
        let sp = stack as u64 + stack_size as u64;
        let mut x = [0; 31];
        x[30] = entry as u64;
        State {
            x,
            pc: entry as u64,
            sp,
        }
    }

    pub fn load(addr: u64) -> Self {
        let mut x = [0; 31];
        for i in 0..31 {
            x[i] = unsafe { read_volatile((addr + i as u64 * 8) as *const u64) };
        }
        let pc = unsafe { read_volatile((addr + 32 * 8) as *const u64) };
        let sp = unsafe { read_volatile((addr + 33 * 8) as *const u64) };
        State { x, pc, sp }
    }

    pub fn store(&self, addr: u64) {
        for i in 0..31 {
            unsafe { write_volatile((addr + i as u64 * 8) as *mut u64, self.x[i]) };
        }
        unsafe { write_volatile((addr + 32 * 8) as *mut u64, self.pc) };
        unsafe { write_volatile((addr + 33 * 8) as *mut u64, self.sp) };
    }
}

impl Debug for State {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "State {{ x: [")?;
        for i in 0..31 {
            write!(f, "{}, ", self.x[i])?;
        }
        write!(f, "], pc: 0x{:x}, sp: 0x{:x} }}", self.pc, self.sp)
    }
}
