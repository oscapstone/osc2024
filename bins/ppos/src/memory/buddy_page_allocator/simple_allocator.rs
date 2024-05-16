use core::{alloc::GlobalAlloc, mem::align_of};

use library::sync::mutex::Mutex;

use crate::memory::{heap_end_addr, heap_start_addr};

struct SimpleAllocatorInner {
    current: usize,
}

impl SimpleAllocatorInner {
    const unsafe fn new() -> Self {
        Self { current: 0 }
    }

    unsafe fn alloc(&mut self, layout: core::alloc::Layout) -> *mut u8 {
        if heap_start_addr() + self.current + layout.size() > heap_end_addr() {
            panic!(
                "Heap memory is not enough to allocate {} bytes",
                layout.size()
            );
        }
        let p = (heap_start_addr() + self.current) as *mut u8;
        self.current += layout.size();
        // align to 8 bytes
        self.current += (self.current as *const u8).align_offset(align_of::<u64>());
        p
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {}
}

pub struct SimpleAllocator {
    inner: Mutex<SimpleAllocatorInner>,
}

impl SimpleAllocator {
    pub const unsafe fn new() -> Self {
        Self {
            inner: Mutex::new(SimpleAllocatorInner::new()),
        }
    }
}

unsafe impl GlobalAlloc for SimpleAllocator {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        let mut inner = self.inner.lock().unwrap();
        inner.alloc(layout)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        let inner = self.inner.lock().unwrap();
        inner.dealloc(ptr, layout);
    }
}

static SIMPLE_ALLOCATOR: SimpleAllocator = unsafe { SimpleAllocator::new() };

pub fn simple_allocator() -> &'static SimpleAllocator {
    &SIMPLE_ALLOCATOR
}
