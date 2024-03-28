const HEAP_SIZE: usize = 1024 * 1024; // 1 MB
const HEAP_START: usize = 0x60000;

use core::cell::UnsafeCell;

mod memory;

use core::alloc::{GlobalAlloc, Layout};
pub struct MyAllocator {
    memory: UnsafeCell<memory::HeapMemory>,
}

impl MyAllocator {
    pub const fn new() -> Self {
        Self {
            memory: UnsafeCell::new(memory::HeapMemory::new(HEAP_START, HEAP_SIZE)),
        }
    }
    unsafe fn memory(&self) -> &mut memory::HeapMemory {
        &mut *self.memory.get()
    }
}

unsafe impl Sync for MyAllocator {}

unsafe impl GlobalAlloc for MyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        memory::alloc(layout, self.memory())
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Implementation for dealloc
    }
}
