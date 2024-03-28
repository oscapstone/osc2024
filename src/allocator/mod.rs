use core::alloc::{GlobalAlloc, Layout};

// use stdio;

struct MyAllocator;

#[global_allocator]
static ALLOCATOR: MyAllocator = MyAllocator;

static mut cur: usize = 0 as usize;

fn align_up(addr: usize, align: usize) -> usize {
    (addr + align - 1) & !(align - 1)
}

unsafe impl GlobalAlloc for MyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();
        let align = layout.align();

        // Calculate total memory needed to ensure alignment
        let total_size = size + align - 1;

        // Use some method to get raw block (this part is very simplified)
        let raw_block = (0x20000000 as *mut u32).add(cur) as *mut u32;
        cur += total_size;

        // Adjust raw_block pointer to meet alignment requirements
        let aligned_address = align_up(raw_block as usize, align);

        // Return the adjusted pointer as *mut u8
        aligned_address as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Implement deallocation logic here
        // stdio::puts(b"You are freeing memory! :(");
    }
}
