use core::alloc::GlobalAlloc;

use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};
use library::{
    console::{console, ConsoleMode},
    println,
    sync::mutex::Mutex,
};

use crate::{memory::PAGE_SIZE, shell::SLAB_ALLOCATOR_DEBUG_ENABLE};

use super::{buddy_page_allocator::BuddyPageAllocator, round_up_with};

/**
 * Slab node
 * The first node stores the next free node. Other nodes store the next node.
 * The first node is stored at bss segment, and the others are stored at heap.
 * When the node is allocated, the node in the heap will be write as 0 to be the allocated memory.
 */
#[derive(Debug, Clone, Copy)]
struct SlabNode(*mut SlabNode);

impl SlabNode {
    /**
     * Allocate size bytes
     * This method will update the next free node
     */
    unsafe fn alloc_one(&mut self, page_allocator: &BuddyPageAllocator, size: usize) -> *mut u8 {
        // if there is no free node, allocate a new page
        if self.0.is_null() {
            let frame =
                page_allocator.alloc(core::alloc::Layout::from_size_align_unchecked(PAGE_SIZE, 4));
            println!("Allocate a new page for {} bytes slab node", size);
            self.init(size, frame);
        }

        // take the first free node
        let res = self.0;
        // update the next free node
        self.0 = (*self.0).0;
        let res = res as *mut u8;
        if SLAB_ALLOCATOR_DEBUG_ENABLE {
            println!("Allocate {} bytes slab node at {:p}", size, res);
        }
        // set the allocated memory to 0 to avoid uninitialized memory issue
        core::slice::from_raw_parts_mut(res, size).fill(0);
        res
    }

    /**
     * Deallocate the memory
     * This method just make the node as the next free node and attach the original next free node to the new free node
     */
    unsafe fn dealloc_one(&mut self, ptr: *mut u8) {
        let ptr = ptr as *mut SlabNode;
        (*ptr).0 = self.0;
        self.0 = ptr;
        if SLAB_ALLOCATOR_DEBUG_ENABLE {
            println!("Deallocate slab node at {:p}", ptr);
        }
    }

    /**
     * Initialize the slab node
     * This method allocates the slab node in a page at once
     */
    unsafe fn init(&mut self, size: usize, ptr: *mut u8) {
        for i in (0..PAGE_SIZE).step_by(size) {
            (ptr.add(i) as *mut SlabNode).write(SlabNode(ptr.add(i + size) as *mut SlabNode));
        }
        // last one
        (ptr.add(PAGE_SIZE - size) as *mut SlabNode).write(SlabNode(core::ptr::null_mut()));
        self.0 = ptr as *mut Self;
    }
}

pub struct SlabAllocator<'a> {
    /**
     * Slab node for each size
     * [8, 16, 32, 64, 128, 256, 512, 1024]
     */
    slab_nodes: Mutex<[SlabNode; SlabAllocator::SLAB_NODE_AMOUNT]>,
    page_allocator: &'a BuddyPageAllocator<'a>,
}

impl<'a> SlabAllocator<'a> {
    const SLAB_NODE_AMOUNT: usize = 8;
    const MAX_SLAB_NODE: usize = 8 - 1;
    const MIN_SLAB_SIZE_SHIFT: usize = 3;
    const MIN_SLAB_SIZE: usize = 1 << Self::MIN_SLAB_SIZE_SHIFT;
    const MAX_SLAB_SIZE: usize = 1024;

    pub const unsafe fn new(page_allocator: &'a BuddyPageAllocator<'a>) -> Self {
        Self {
            slab_nodes: Mutex::new([SlabNode(core::ptr::null_mut()); Self::MAX_SLAB_NODE + 1]),
            page_allocator,
        }
    }

    /**
     * Get the size and index of the slab node from the requested size
     */
    fn get_size_and_index(size: usize) -> (usize, usize) {
        let size = round_up_with(size.next_power_of_two(), Self::MIN_SLAB_SIZE);
        let index = (size >> Self::MIN_SLAB_SIZE_SHIFT).trailing_zeros() as usize;
        (size, index)
    }
}

unsafe impl<'a> GlobalAlloc for SlabAllocator<'a> {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        disable_kernel_space_interrupt();
        console().change_mode(ConsoleMode::Sync);
        if layout.size() > Self::MAX_SLAB_SIZE {
            let ptr = self.page_allocator.alloc(layout);
            console().change_mode(ConsoleMode::Async);
            enable_kernel_space_interrupt();
            return ptr;
        }
        let (size, index) = Self::get_size_and_index(layout.size());
        let ptr = self.slab_nodes.lock().unwrap()[index].alloc_one(&self.page_allocator, size);
        console().change_mode(ConsoleMode::Async);
        enable_kernel_space_interrupt();
        ptr
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        disable_kernel_space_interrupt();
        console().change_mode(ConsoleMode::Sync);
        if layout.size() > Self::MAX_SLAB_SIZE {
            self.page_allocator.dealloc(ptr, layout);
            console().change_mode(ConsoleMode::Async);
            enable_kernel_space_interrupt();
            return;
        }

        let (_, index) = Self::get_size_and_index(layout.size());
        self.slab_nodes.lock().unwrap()[index].dealloc_one(ptr);
        console().change_mode(ConsoleMode::Async);
        enable_kernel_space_interrupt();
    }
}
