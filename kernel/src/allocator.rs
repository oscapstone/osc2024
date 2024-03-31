use core::alloc::{GlobalAlloc, Layout};

struct BumpAllocator {
    // heap_start: *const u8,
    // heap_end: *const u8,
    // root_addr: *const u8,
}

#[global_allocator]
static ALLOCATOR: BumpAllocator = BumpAllocator {
    // heap_start: 0x1000_0000 as *const u8,
    // heap_end: 0x3000_0000 as *const u8,
    // root_addr: 0x1000_0000 as *const u8,
};

static mut CUR: usize = 0x1000_0000;

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();
        let align = layout.align();
        let ret = CUR;
        CUR += size;
        CUR += align - 1;
        CUR &= !(align - 1);

        // stdio::print("Allocating: ");
        // stdio::print_u32(ret as u32);
        // stdio::print(" ");
        // stdio::print_u32(size as u32);
        // stdio::println("");

        ret as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Do nothing
    }
}
