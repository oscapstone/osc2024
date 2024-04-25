use core::alloc::{AllocError, Allocator, GlobalAlloc, Layout};
use core::ptr::NonNull;

use stdio::debug;

#[derive(Clone)]
pub struct BumpAllocator;

static mut CUR: usize = 0x2000_0000;

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();
        let align = layout.align();
        let ret = CUR;
        CUR += size;
        CUR = (CUR + align - 1) & !(align - 1);
        assert!(CUR < 0x2000_1000, "Bump allocator out of memory!");
        debug!("Allocated {} bytes at 0x{:x}", size, ret);
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
