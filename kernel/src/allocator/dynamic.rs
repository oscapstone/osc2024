use super::buddy::BUDDY_SYSTEM;
use core::alloc::{GlobalAlloc, Layout};

#[global_allocator]
static mut DYNAMIC_ALLOCATOR: DynamicAllocator = DynamicAllocator;

pub struct DynamicAllocator;

unsafe impl GlobalAlloc for DynamicAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        BUDDY_SYSTEM.alloc(layout)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        BUDDY_SYSTEM.dealloc(ptr, layout)
    }
}
