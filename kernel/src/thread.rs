pub mod cpu;
pub mod state;

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
        let stack = vm.mmap(0xffff_ffff_b000, stack_size, 0b0100_0100_0111) as *mut u8;
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + stack_size
        );
        let pc = vm.map_pa(0x0000_0000_0000, entry as u64, len, 0b0100_1100_0111);
        // vm.map_pa(0x3C00_0000, 0x3C00_0000, 0x400_0000, 0b0100_0100_0111);
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
        // self.vm.dump();
        let mut vm = VirtualMemory::new();
        let stack = vm.mmap(0xffff_ffff_b000, self.stack_size, 0b0100_0100_0111) as *mut u8;

        let src = self.vm.get_phys(self.stack as u64);
        let dst = vm.get_phys(stack as u64);
        println!(
            "Cloning stack 0x{:x} to 0x{:x}, size: 0x{:x}",
            src as usize, dst as usize, self.stack_size
        );
        unsafe {
            core::ptr::copy_nonoverlapping(
                self.vm.get_phys(self.stack as u64),
                vm.get_phys(stack as u64),
                self.stack_size,
            );
        }
        let mut cpu_state = self.cpu_state.clone();
        cpu_state.l0 = vm.get_l0_addr() as u64;
        vm.map_pa(
            0x0000_0000_0000,
            self.entry as u64,
            self.len,
            0b0100_1100_0111,
        );
        vm.map_pa(0x3C00_0000, 0x3C00_0000, 0x400_0000, 0b0100_0100_0111);
        println!(
            "Cloning thread 0x{:x} to 0x{:x}",
            self.id, 0xdeadbeaf as u32
        );
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + self.stack_size
        );
        // vm.dump();
        Thread {
            id: 0xdeadbeaf,
            stack,
            cpu_state,
            vm,
            ..*self
        }
    }
}
