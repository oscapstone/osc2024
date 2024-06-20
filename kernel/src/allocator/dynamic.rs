use super::buddy::BUDDY_SYSTEM;
use super::bump::BumpAllocator;
use super::config::FRAME_SIZE;
use alloc::vec::Vec;
use alloc::{collections::BTreeMap, vec};
use core::alloc::{GlobalAlloc, Layout};
use stdio::{debug, println};

#[global_allocator]
pub static mut DYNAMIC_ALLOCATOR: DynamicAllocator = DynamicAllocator::new();

#[allow(dead_code)]
pub fn toggle_verbose() {
    unsafe {
        DYNAMIC_ALLOCATOR.verbose = !DYNAMIC_ALLOCATOR.verbose;
    }
    println!("DynamicAllocator verbose: {}", unsafe {
        DYNAMIC_ALLOCATOR.verbose
    });
}

pub struct DynamicAllocator {
    data: BTreeMap<(usize, usize), Vec<*mut u8, BumpAllocator>, BumpAllocator>,
    verbose: bool,
}

impl DynamicAllocator {
    const fn new() -> Self {
        DynamicAllocator {
            data: BTreeMap::new_in(BumpAllocator),
            verbose: false,
        }
    }
}

fn align_up(addr: usize, align: usize) -> usize {
    (addr + align - 1) & !(align - 1)
}

unsafe fn slice_page(size: usize, align: usize) -> vec::Vec<*mut u8, BumpAllocator> {
    assert!(
        size <= FRAME_SIZE,
        "Cannot slice page into more than {} bytes",
        FRAME_SIZE
    );
    let mut ret = vec::Vec::new_in(BumpAllocator);
    let mut ptr = BUDDY_SYSTEM.alloc(Layout::from_size_align(FRAME_SIZE, align).unwrap());
    if DYNAMIC_ALLOCATOR.verbose {
        debug!(
            "Slicing page at 0x{:x} into {} bytes, align {}",
            ptr as usize, size, align
        );
    }
    for _ in 0..FRAME_SIZE / size {
        ret.push(ptr);
        ptr = (ptr as usize + size) as *mut u8;
        ptr = align_up(ptr as usize, align) as *mut u8;
    }
    if DYNAMIC_ALLOCATOR.verbose {
        debug!("Sliced vector size: {}", ret.len());
    }
    ret
}

unsafe impl GlobalAlloc for DynamicAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if BUDDY_SYSTEM.initialized == false {
            return BumpAllocator.alloc(layout);
        }
        if layout.size() > FRAME_SIZE {
            if DYNAMIC_ALLOCATOR.verbose {
                debug!("Allocating {} bytes with size > 0x1000", layout.size());
            }
            return BUDDY_SYSTEM.alloc(layout);
        }
        if DYNAMIC_ALLOCATOR.verbose {
            println!("Allocating {} bytes with size <= 0x1000", layout.size());
        }
        let size = layout.size();
        let align = layout.align();
        let key = (size, align);
        let ptr = match DYNAMIC_ALLOCATOR.data.get_mut(&key) {
            Some(v) => {
                if let Some(ptr) = v.pop() {
                    ptr
                } else {
                    DYNAMIC_ALLOCATOR.data.insert(key, slice_page(size, align));
                    DYNAMIC_ALLOCATOR.alloc(layout)
                }
            }
            None => {
                if DYNAMIC_ALLOCATOR.verbose {
                    println!("DynamicAllocator::alloc: vector not found");
                }
                DYNAMIC_ALLOCATOR.data.insert(key, slice_page(size, align));
                if DYNAMIC_ALLOCATOR.verbose {
                    println!("DynamicAllocator::alloc: vector inserted");
                }
                DYNAMIC_ALLOCATOR.alloc(layout)
            }
        };
        if DYNAMIC_ALLOCATOR.verbose {
            println!(
                "DynamicAllocator::alloc: 0x{:x} size {} align {}",
                ptr as usize, size, align
            );
        }
        ptr
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        if BUDDY_SYSTEM.initialized == false {
            return BumpAllocator.dealloc(ptr, layout);
        }
        if layout.size() > FRAME_SIZE {
            if DYNAMIC_ALLOCATOR.verbose {
                debug!(
                    "Deallocating 0x{:x} with size > {}",
                    ptr as usize, FRAME_SIZE
                );
            }
            return BUDDY_SYSTEM.dealloc(ptr, layout);
        }
        if DYNAMIC_ALLOCATOR.verbose {
            println!(
                "Deallocating 0x{:x} with {} bytes",
                ptr as usize,
                layout.size()
            );
        }
        let size = layout.size();
        let align = layout.align();
        let key = (size, align);
        DYNAMIC_ALLOCATOR.data.get_mut(&key).unwrap().push(ptr);
    }
}
