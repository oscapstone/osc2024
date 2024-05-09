use super::config::{BUMP_END_ADDR, BUMP_START_ADDR};
use core::alloc::{AllocError, Allocator, GlobalAlloc, Layout};
use core::ptr::NonNull;
#[allow(unused_imports)]
use stdio::debug;
use stdio::println;

#[derive(Clone)]
pub struct BumpAllocator;

static mut VERBOSE: bool = false;

static mut CUR: u32 = BUMP_START_ADDR;

#[allow(dead_code)]
pub fn toggle_verbose() {
    unsafe {
        VERBOSE = !VERBOSE;
    }
    println!("BumpAllocator verbose: {}", unsafe { VERBOSE });
}

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size() as u32;
        let align = layout.align() as u32;
        let mut ret = CUR;
        ret = (ret + align - 1) & !(align - 1);
        CUR = ret + size;
        assert!(CUR < BUMP_END_ADDR, "Bump allocator out of memory!");
        if unsafe { VERBOSE } {
            debug!(
                "BumpAllocator: alloc 0x{:x} size 0x{:x} align 0x{:x}",
                ret, size, align
            );
        }
        ret as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Do nothing
    }
}

unsafe impl Allocator for BumpAllocator {
    fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
        unsafe {
            let ptr = self.alloc(layout);
            NonNull::new(ptr)
                .map(|p| NonNull::slice_from_raw_parts(p, layout.size()))
                .ok_or(AllocError)
        }
    }

    unsafe fn deallocate(&self, _ptr: NonNull<u8>, _layout: Layout) {
        // Custom deallocation logic here
    }
}
