use core::cell::UnsafeCell;
use core::panic;
use core::alloc::{GlobalAlloc, Layout};


use super::buddy::MemoryManager;
use super::best_fit::BestFitAllocator;


pub struct DynamicMemoryAllocator {
    memory: UnsafeCell<Option<MemoryManager>>,
    best_fit_alloc: UnsafeCell<Option<BestFitAllocator>>,
    page_size: usize,
}

struct MemBlock {
    pn: usize,
    size: usize,
}

impl DynamicMemoryAllocator {
    pub const fn new() -> Self {
        Self {
            memory: UnsafeCell::new(Option::None),
            best_fit_alloc: UnsafeCell::new(Option::None),
            page_size: 0,
        }
    }
    
    pub fn init(&mut self, mem_start: usize, mem_end: usize, page_size: usize) {
        self.memory = Some(MemoryManager::new(mem_start, mem_end, page_size)).into();
        // // first allocate some pages for best fit allocator
        // let bf_addr = unsafe {(*self.memory.get()).as_mut().unwrap().alloc(16)};
        // self.best_fit_alloc = Some(BestFitAllocator::new(bf_addr as usize, bf_addr as usize + 16 * page_size)).into();
        // self.page_size = page_size;
    }
    // return page number
    pub fn palloc(&mut self, p_size: usize) -> usize{
        unsafe {(*self.memory.get()).as_mut().unwrap().get_free_block(p_size)}
    }
    // return memory address
    pub fn page_alloc(&mut self, p_size: usize) -> *mut u8 {
        unsafe {(*self.memory.get()).as_mut().unwrap().alloc(p_size)}
    }

    // free a page
    pub fn pfree(&mut self,pn: usize){
        unsafe {(*self.memory.get()).as_mut().unwrap().free_page(pn)}
    }

    pub fn pshow(&self) {
        unsafe {(*self.memory.get()).as_ref().unwrap().show()};   
    }

    pub fn dshow(&self) {
        // unsafe {(*self.best_fit_alloc.get()).as_ref().unwrap().show()};   
    }

    pub fn reserve_addr(&mut self, start: usize, end: usize) {
        unsafe {(*self.memory.get()).as_mut().unwrap().reserve_addr(start, end)};
    }

}

unsafe impl Sync for DynamicMemoryAllocator {}
// best fit allocator
unsafe impl GlobalAlloc for DynamicMemoryAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if (*self.memory.get()).is_none() || (*self.best_fit_alloc.get()).is_none() {
            panic!("MemoryManager is not initialized");
        }
        match (*self.best_fit_alloc.get()).as_mut().unwrap().alloc(layout) {
            Some(addr) => addr,
            None => {
                panic!("DynMemAllocator: Out of memory");
            }
        }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        (*self.best_fit_alloc.get()).as_mut().unwrap().dealloc(ptr);
    }
}