/*
inspired from https://os.phil-opp.com
*/
use crate::synchronization::{interface::Mutex, NullLock};
// use crate::{print, println};

use core::alloc::{GlobalAlloc, Layout};
use crate::println;

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

pub struct BumpAllocator {
    inner: NullLock<BumpAllocatorInner>,
}

struct BumpAllocatorInner {
    heap_start: usize,
    heap_end: usize,
    next: usize,
    allocations: usize,
}

//--------------------------------------------------------------------------------------------------
// Private Code
//--------------------------------------------------------------------------------------------------

impl BumpAllocatorInner {
    // Creates a new empty bump allocator.
    pub const fn new() -> Self {
        BumpAllocatorInner {
            heap_start: 0,
            heap_end: 0,
            next: 0,
            allocations: 0,
        }
    }
    /// Initializes the bump allocator with the given heap bounds.
    ///
    /// This method is unsafe because the caller must ensure that the given
    /// memory range is unused. Also, this method must be called only once.
    pub unsafe fn init(&mut self, heap_start: usize, heap_size: usize) {
        self.heap_start = heap_start;
        self.heap_end = heap_start + heap_size;
        self.next = heap_start;
    }

    fn alloc(&mut self, layout: Layout) -> *mut u8 {
        println!("START alloc");
        let alloc_start = self.next;
        self.next = alloc_start + layout.size();
        self.allocations += 1;
        alloc_start as *mut u8
    }

    #[allow(dead_code)]
    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        todo!();
    }
}

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

impl BumpAllocator {
    #[allow(dead_code)]
    pub const COMPATIBLE: &'static str = "MEMORY_ALLOCATOR";
    /// Create an instance.
    ///
    /// # Safety
    pub const fn new() -> Self {
        Self {
            inner: NullLock::new(BumpAllocatorInner::new()),
        }
    }

    pub unsafe fn init(&self, heap_start: usize, heap_size: usize) {
        self.inner.lock(|inner| inner.init(heap_start, heap_size));
    }
}

//------------------------------------------------------------------------------
// OS Interface Code
//------------------------------------------------------------------------------

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        self.inner.lock(|inner| inner.alloc(layout))
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // todo!()
    }
}
