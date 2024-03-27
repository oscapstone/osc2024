use super::stdio::*;
use core::alloc::{GlobalAlloc, Layout};
use core::ptr::{null, null_mut};

#[global_allocator]
static ALLOCATOR: Allocator = Allocator;

struct Header {
    size: usize,
    start: *mut u8,
    next: *mut Header,
}

struct Allocator;

unsafe impl GlobalAlloc for Allocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if layout.size() == 0 {
            println("Warning: Allocating zero size");
            return null_mut();
        }
        let mut current = 0x3000_0000 as *mut Header;
        loop {
            if current.is_null() {
                // At the end of the list
                current = 0x3000_0000 as *mut Header;
                *current = Header {
                    size: layout.size(),
                    start: current.add(1) as *mut u8,
                    next: null_mut(),
                };

                // check alignment
                let offset = (*current).start.align_offset(layout.align());
                (*current).start = (*current).start.add(offset);

                break;

            // TODO: Find the block that fits the layout
            // } else if (*current).start.add((*current).size).add(layout.size()) < (*current).next. {
            //     let start = (*current).start.add((*current).size);
            //     (*current).size += layout.size();
            //     return start;
            } else {
                current = (*current).next;
            }
        }

        assert_ne!(current, null_mut());
        (*current).start
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        // Implement deallocation logic here
        // null();
    }
}
