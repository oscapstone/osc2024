use core::alloc::{GlobalAlloc, Layout};

use small_std::{println, sync::Mutex};

extern "C" {
    pub static _heap_start: usize;
    pub static _heap_end_exclusive: usize;
}

/// A simple allocator that allocates memory from a fixed-size arena.
struct ArenaAllocator {
    current_offset: Mutex<usize>,
}

impl ArenaAllocator {
    const fn new() -> Self {
        Self {
            current_offset: Mutex::new(0),
        }
    }

    fn compute_alloc_region(&self, layout: Layout) -> (usize, usize) {
        let current_offset = self.current_offset.lock().unwrap();
        let head = self.address_at(*current_offset);
        let size = layout.size();
        let align = layout.align();

        let start = unsafe { head.add(head.align_offset(align)) } as usize;
        let end = start + size;

        (start, end)
    }

    fn is_region_valid(&self, start: usize, end: usize) -> bool {
        start >= self.heap_start() && end <= self.heap_end()
    }

    fn bump(&self, end: usize) {
        let new_offset = end - self.heap_start();
        let mut current_offset = self.current_offset.lock().unwrap();
        *current_offset = new_offset;
    }

    #[inline(always)]
    fn heap_start(&self) -> usize {
        unsafe { &_heap_start as *const usize as usize }
    }

    #[inline(always)]
    fn heap_end(&self) -> usize {
        unsafe { &_heap_end_exclusive as *const usize as usize }
    }

    #[inline(always)]
    fn address_at(&self, offset: usize) -> *mut u8 {
        (self.heap_start() + offset) as *mut u8
    }
}

unsafe impl GlobalAlloc for ArenaAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let (start, end) = self.compute_alloc_region(layout);
        println!(
            "Allocating {:?} at {:p} - {:p}",
            layout, start as *const u8, end as *const u8
        );
        println!(
            "Heap start: {:p}, end: {:p}",
            self.heap_start() as *const u8,
            self.heap_end() as *const u8
        );
        if !self.is_region_valid(start, end) {
            return core::ptr::null_mut();
        }

        self.bump(end);
        start as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // This allocator never deallocates memory
    }
}

#[global_allocator]
static ALLOCATOR: ArenaAllocator = ArenaAllocator::new();
