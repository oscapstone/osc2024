pub mod cpu;
pub mod state;

use crate::mmu::vm::VirtualMemory;
// use alloc::alloc::alloc;
// use core::alloc::Layout;
use stdio::println;

#[repr(C)]
#[derive(Debug)]
pub struct Thread {
    pub id: usize,
    pub state: state::State,
    pub stack: *mut u8,
    pub stack_size: usize,
    pub entry: *mut u8,
    pub cpu_state: cpu::State,
    pub vm: VirtualMemory,
}

impl Thread {
    pub fn new(stack_size: usize, entry: *mut u8, len: usize) -> Self {
        let mut vm = VirtualMemory::new();
        // let stack =
        //     unsafe { alloc(Layout::from_size_align(stack_size, 0x100).unwrap()) as *mut u8 };
        let stack = vm.mmap(0x7fff_ffff_b000, stack_size, 0b0100_0100_0111) as *mut u8;
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + stack_size
        );
        let pc = vm.mmap(0x0000_0000_0000, len, 0b0100_1100_0111);
        let phy = vm.get_phys(pc as u64);
        println!("Physical address: 0x{:x}", phy as usize);
        println!("entry: 0x{:x}, len: 0x{:x}", entry as usize, len);
        vm.dump();

        unsafe {
            core::ptr::copy(entry, phy as *mut u8, len);
        }
        // unsafe {
        //     for i in 0..len {
        //         let val = core::ptr::read(entry.add(i));
        //         core::ptr::write(phy.add(i), val);
        //         println!("0x{:x} = 0x{:x}", phy as usize + i, val);
        //     }
        // }
        assert!(stack == 0x7fff_ffff_b000 as *mut u8);
        let cpu_state = cpu::State::new(stack, stack_size, entry, vm.get_l0_addr());
        println!(
            "Stack: {:x}-{:x}",
            stack as usize,
            stack as usize + stack_size
        );
        println!("entry: 0x{:x}, len: 0x{:x}", entry as usize, len);
        Thread {
            id: 0xC8763,
            state: state::State::Ready,
            stack,
            stack_size,
            entry: pc,
            cpu_state,
            vm,
        }
    }
}

impl Clone for Thread {
    fn clone(&self) -> Self {
        // let stack =
        //     unsafe { alloc(Layout::from_size_align(self.stack_size, 0x100).unwrap()) as *mut u8 };
        panic!("Cloning thread 0x{:x}", self.id);
        // let mut vm = self.vm.clone();
        // let stack = vm.mmap(0x7fff_ffff_0000, self.stack_size) as *mut u8;
        // unsafe {
        //     core::ptr::copy(self.stack, stack, self.stack_size);
        // }
        // let mut cpu_state = self.cpu_state.clone();
        // cpu_state.sp = (cpu_state.sp as usize - self.stack as usize + stack as usize) as u64;
        // println!(
        //     "Cloning thread 0x{:x} to 0x{:x}",
        //     self.id, 0xdeadbeaf as u32
        // );
        // println!(
        //     "Stack: {:x}-{:x}",
        //     stack as usize,
        //     stack as usize + self.stack_size
        // );
        // Thread {
        //     id: 0xdeadbeaf,
        //     stack,
        //     cpu_state,
        //     vm,
        //     ..*self
        // }
    }
}
