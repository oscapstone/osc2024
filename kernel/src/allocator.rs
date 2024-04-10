use core::alloc::{GlobalAlloc, Layout};

struct BumpAllocator;

#[global_allocator]
static ALLOCATOR: BumpAllocator = BumpAllocator;

static mut CUR: usize = 0x1000_0000;

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();
        let align = layout.align();
        let ret = CUR;
        CUR += size;
        CUR = (CUR + align - 1) & !(align - 1);
        ret as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Do nothing
    }
}
