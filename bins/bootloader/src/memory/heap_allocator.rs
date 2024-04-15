use core::{alloc::GlobalAlloc, mem::align_of};

use crate::memory;
use library::sync::mutex::Mutex;

#[global_allocator]
static KERNEL_HEAP_ALLOCATOR: HeapAllocator = unsafe { HeapAllocator::new() };

struct HeapAllocatorInner {
    current: usize,
}

impl HeapAllocatorInner {
    const unsafe fn new() -> Self {
        Self { current: 0 }
    }

    unsafe fn alloc(&mut self, layout: core::alloc::Layout) -> *mut u8 {
        if Self::heap_start_addr() + self.current + layout.size() > Self::heap_end_addr() {
            panic!(
                "Heap memory is not enough to allocate {} bytes",
                layout.size()
            );
        }
        let p = (Self::heap_start_addr() + self.current) as *mut u8;
        self.current += layout.size();
        // align to 8 bytes
        self.current += (self.current as *const u8).align_offset(align_of::<u64>());
        p
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {}

    #[inline(always)]
    unsafe fn heap_start_addr() -> usize {
        &memory::__heap_begin as *const usize as usize
    }

    #[inline(always)]
    unsafe fn heap_end_addr() -> usize {
        &memory::__heap_end as *const usize as usize
    }
}

pub struct HeapAllocator {
    inner: Mutex<HeapAllocatorInner>,
}

impl HeapAllocator {
    pub const unsafe fn new() -> Self {
        Self {
            inner: Mutex::new(HeapAllocatorInner::new()),
        }
    }
}

unsafe impl GlobalAlloc for HeapAllocator {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        let mut inner = self.inner.lock().unwrap();
        inner.alloc(layout)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        let inner = self.inner.lock().unwrap();
        inner.dealloc(ptr, layout);
    }
}
