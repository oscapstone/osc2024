#[repr(C)]
#[derive(Clone, Copy)]
pub struct State {
    x19: u64,
    x20: u64,
    x21: u64,
    x22: u64,
    x23: u64,
    x24: u64,
    x25: u64,
    x26: u64,
    x27: u64,
    x28: u64,
    fp: u64,
    lr: u64,
    sp: u64,
    sp_el0: u64,
    elr_el1: u64,
    ttbr0_el1: u64,
}

impl State {
    pub fn new(stack: *mut u8, entry: extern "C" fn()) -> Self {
        let sp = stack as u64;
        let sp_el0 = stack as u64;
        let elr_el1 = entry as u64;
        let ttbr0_el1 = 0x80000;
        State {
            x19: 0,
            x20: 0,
            x21: 0,
            x22: 0,
            x23: 0,
            x24: 0,
            x25: 0,
            x26: 0,
            x27: 0,
            x28: 0,
            fp: 0,
            lr: entry as u64,
            sp,
            sp_el0,
            elr_el1,
            ttbr0_el1,
        }
    }
}
