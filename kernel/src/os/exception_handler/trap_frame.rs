use core::ptr::{read_volatile, write_volatile};

pub enum TrapFrameRegister {
    X0 = 0,
    X1 = 1,
    X2 = 2,
    X3 = 3,
    X4 = 4,
    X5 = 5,
    X6 = 6,
    X7 = 7,
    X8 = 8,
    X9 = 9,
    X10 = 10,
    X11 = 11,
    X12 = 12,
    X13 = 13,
    X14 = 14,
    X15 = 15,
    X16 = 16,
    X17 = 17,
    X18 = 18,
    X19 = 19,
    X20 = 20,
    X21 = 21,
    X22 = 22,
    X23 = 23,
    X24 = 24,
    X25 = 25,
    X26 = 26,
    X27 = 27,
    X28 = 28,
    X29 = 29,
    X30 = 30,
}

pub unsafe fn get(trap_frame_ptr: *mut u64, reg: TrapFrameRegister) -> u64 {
    let reg_ptr = trap_frame_ptr.add(reg as usize);
    read_volatile(reg_ptr as *const u64)
}

pub unsafe fn set(trap_frame_ptr: *mut u64, reg: TrapFrameRegister, value: u64) {
    let reg_ptr = trap_frame_ptr.add(reg as usize);
    write_volatile(reg_ptr as *mut u64, value);
}
