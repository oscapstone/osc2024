pub mod cpu;
pub mod state;

use crate::mmu::config::GPU_CONFIG;
use crate::mmu::config::STACK_CONFIG;
use crate::mmu::config::TEXT_CONFIG;
use crate::mmu::vm::VirtualMemory;
use stdio::println;

#[repr(C)]
#[derive(Debug)]
pub struct Thread {
    pub id: usize,
    pub state: state::State,
    pub stack: *mut u8,
    pub stack_size: usize,
    pub entry: *mut u8,
    pub len: usize,
    pub cpu_state: cpu::State,
    pub vm: VirtualMemory,
}

impl Thread {
    pub fn new(stack_size: usize, entry: *mut u8, len: usize) -> Self {
        let mut vm = VirtualMemory::new();
        let stack = vm.mmap(0xffff_ffff_b000, stack_size, STACK_CONFIG) as *mut u8;
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + stack_size
        );
        let pc = vm.map_pa(0x0000_0000_0000, entry as u64, len, TEXT_CONFIG);
        vm.map_pa(0x3C00_0000, 0x3C00_0000, 0x400_0000, GPU_CONFIG);
        assert!(stack == 0xffff_ffff_b000 as *mut u8);
        let cpu_state = cpu::State::new(stack, stack_size, pc, vm.get_l0_addr());
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + stack_size
        );
        println!("pc: {:x}", pc as usize);
        Thread {
            id: 0xC8763,
            state: state::State::Ready,
            stack,
            stack_size,
            entry,
            len,
            cpu_state,
            vm,
        }
    }
}

impl Clone for Thread {
    fn clone(&self) -> Self {
        let mut vm = VirtualMemory::new();
        let stack = vm.mmap(0xffff_ffff_b000, self.stack_size, STACK_CONFIG) as *mut u8;
        unsafe {
            core::ptr::copy_nonoverlapping(
                self.vm.get_phys(self.stack as u64),
                vm.get_phys(stack as u64),
                self.stack_size,
            );
        }
        let mut cpu_state = self.cpu_state.clone();
        cpu_state.l0 = vm.get_l0_addr() as u64;
        vm.map_pa(0x0000_0000_0000, self.entry as u64, self.len, TEXT_CONFIG);
        vm.map_pa(0x3C00_0000, 0x3C00_0000, 0x400_0000, GPU_CONFIG);
        println!(
            "Cloning thread 0x{:x} to 0x{:x}",
            self.id, 0xdeadbeaf as u32
        );
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + self.stack_size
        );
        Thread {
            id: 0xdeadbeaf,
            stack,
            cpu_state,
            vm,
            ..*self
        }
    }
}
