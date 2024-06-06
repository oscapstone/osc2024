use core::{alloc::{Layout, GlobalAlloc}, panic};
const HEAP_SIZE: usize = 0xA00000; // 10 MB
const HEAP_START: usize = 0x200000;

use core::cell::UnsafeCell;

pub struct HeapMemory {
    heap_start: usize,
    current: usize,
    heap_size: usize,
}

impl HeapMemory {
    pub const fn new(heap_start: usize, heap_size: usize) -> Self {
        Self {
            heap_start,
            current: heap_start,
            heap_size,
        }
    }
}

fn alloc(layout: Layout, memory: &mut HeapMemory) -> *mut u8 {
    let size = layout.size();
    let align = layout.align();
    let new_current = (memory.current + align - 1) & !(align - 1);
    if new_current + size > memory.heap_start + memory.heap_size {
        panic!("Simple Allocator: Out of memory");
    }
    memory.current = new_current + size;
    new_current as *mut u8
}

pub struct SimpleAllocator {
    memory: UnsafeCell<HeapMemory>,
}

impl SimpleAllocator {
    pub const fn new() -> Self {
        Self {
            memory: UnsafeCell::new(HeapMemory::new(HEAP_START, HEAP_SIZE)),
        }
    }

    unsafe fn memory(&self) -> &mut HeapMemory {
        &mut *self.memory.get()
    }
}

unsafe impl Sync for SimpleAllocator {}

unsafe impl GlobalAlloc for SimpleAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        alloc(layout, self.memory())
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Implementation for dealloc
    }
}
