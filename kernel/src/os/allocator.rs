use self::buddy_system::BuddyAllocator;

use super::stdio::*;
use crate::println;
use core::alloc::{AllocError, Allocator, GlobalAlloc, Layout};
use core::ptr::{null_mut, NonNull};

mod buddy_system;

#[global_allocator]
pub static mut ALLOCATOR: BuddyAllocator = BuddyAllocator {};

pub static mut SIMPLE_ALLOCATOR: SimpleAllocator = SimpleAllocator {};

pub unsafe fn init() {
    buddy_system::init();
}

pub fn reserve(ptr: *mut u8, size: usize) {
    buddy_system::reserve(ptr, size);
}

pub struct SimpleAllocator;

impl SimpleAllocator {}

unsafe impl GlobalAlloc for SimpleAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        static mut count: u32 = 4;
        if layout.size() == 0 {
            println("Warning: Allocating zero size");
            return null_mut();
        }
        let start = 0x0910_0000 as *mut u8;

        let aligned_start = start.add(count as usize).align_offset(layout.align());
        let allocated_start = start.add(count as usize + aligned_start);

        count += (layout.size() + aligned_start) as u32;

        if allocated_start as u32 > 0x1000_0000 {
            println!("Out of memory: {:x}", allocated_start as u32);
            panic!("Out of memory");
        }

        //println_now("SA: allocated");

        allocated_start
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {}
}

unsafe impl Allocator for SimpleAllocator {
    fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
        unsafe {
            let ptr = self.alloc(layout);
            NonNull::new(ptr)
                .map(|p| NonNull::slice_from_raw_parts(p, layout.size()))
                .ok_or(AllocError)
        }
    }

    unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {}
}
