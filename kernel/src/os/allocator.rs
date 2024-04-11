use super::stdio::*;
use core::alloc::{GlobalAlloc, Layout};
use core::ptr::{null, null_mut};
use crate::println;

#[global_allocator]
pub static ALLOCATOR: Allocator = Allocator;

struct Header {
    size: usize,
    start: *mut u8,
    next: *mut Header,
}

pub struct Allocator;

impl Allocator {
}


unsafe impl GlobalAlloc for Allocator {
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

        if allocated_start as u32 > 0x2000_0000 {
            println!("Out of memory: {:x}", allocated_start as u32);
            panic!("Out of memory");
        }

        allocated_start

        // start.add(count as usize) as *mut u8
        // let mut current = 0x3001_0000 as *mut Header;
        // loop {
        //     println("Looping...");
        //     if (*current).next.is_null() {
        //         println("End of list");
        //         // At the end of the list
        //         (*current).next = (*current).start.add((*current).size + 10) as *mut Header;
        //         println("Moving to next");
        //         current = (*current).next;
        //         println("Setting header");
        //         (*current).size = layout.size();
        //         (*current).start = current.add(2) as *mut u8;
        //         (*current).next = null_mut();
        //         println("check alignment");
        //         // check alignment
        //         let offset = (*current).start.align_offset(layout.align());
        //         (*current).start = (*current).start.add(offset);

        //         break;

        //     // TODO: Find the block that fits the layout
        //     // } else if (*current).start.add((*current).size).add(layout.size()) < (*current).next. {
        //     //     let start = (*current).start.add((*current).size);
        //     //     (*current).size += layout.size();
        //     //     return start;
        //     } else {
        //         println("Not end of list");
        //         current = (*current).next;
        //     }
        // }

        // assert_ne!(current, null_mut());
        // print("Allocating ");
        // print_hex((*current).start as u32);
        // (*current).start
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        // Implement deallocation logic here
        // println("Deallocating memory");
        // null();
    }
}
