use core::alloc::Layout;

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

pub fn alloc(layout: Layout, memory: &mut HeapMemory) -> *mut u8 {
    let size = layout.size();
    let align = layout.align();
    let new_current = (memory.current + align - 1) & !(align - 1);
    if new_current + size > memory.heap_start + memory.heap_size {
        return core::ptr::null_mut();
    }
    memory.current = new_current + size;
    new_current as *mut u8
}

