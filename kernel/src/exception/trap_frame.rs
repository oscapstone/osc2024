use crate::thread::cpu;
pub static mut TRAP_FRAME: Option<TrapFrame> = None;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct TrapFrame {
    pub state: cpu::State,
    addr: u64,
}

impl TrapFrame {
    pub fn new(addr: u64) -> Self {
        let state = cpu::State::load(addr);
        TrapFrame { state, addr }
    }
    pub fn restore(&self) {
        self.state.store(self.addr);
    }
}
