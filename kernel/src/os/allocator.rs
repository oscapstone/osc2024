use self::buddy_system::BuddyAllocator;

use super::stdio::*;
use crate::println;
use core::alloc::{AllocError, Allocator, GlobalAlloc, Layout};
use core::ptr::{null_mut, NonNull};

mod buddy_system;

#[global_allocator]
pub static mut ALLOCATOR: BuddyAllocator = BuddyAllocator {};

pub static mut SIMPLE_ALLOCATOR: SimpleAllocator = SimpleAllocator {};

static mut SA_ADDRESS: *mut u8 = 0 as *mut u8;

pub fn set_print_debug(status: bool) {
    buddy_system::set_print_debug(status);
}

pub unsafe fn init() {
    SA_ADDRESS = 0x0A00_0000 as *mut u8;
    buddy_system::init(0x3C00_0000);
}

pub unsafe fn reserve(ptr: *mut u8, size: usize) {
    buddy_system::reserve(ptr, size);
}

pub struct SimpleAllocator;

impl SimpleAllocator {}

unsafe impl GlobalAlloc for SimpleAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        
        //print_hex_now(count);
        if layout.size() == 0 {
            println("Warning: Allocating zero size");
            return null_mut();
        }

        let aligned_start = SA_ADDRESS.align_offset(layout.align());
        let allocated_start = SA_ADDRESS.add(aligned_start);


        SA_ADDRESS = allocated_start.add(layout.size());

        if allocated_start as usize > 0x1000_0000 {
            println!("Out of memory: {:x}", allocated_start as u32);
            panic!("Out of memory");
        }

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
