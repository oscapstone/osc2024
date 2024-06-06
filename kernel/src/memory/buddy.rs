use alloc::collections::btree_set::BTreeSet;
use alloc::vec;
use alloc::vec::Vec;

use core::panic;

use crate::println;
#[derive(Clone, Debug)]
enum PageState {
    Free(usize),  // free block head
    Used(usize),  // used block head
    Buddy(usize), // part of other block, save the head page number
    Rsv,          // reserved
}

pub struct MemoryManager {
    mem_addr_start: usize,
    mem_addr_end: usize,
    mem_addr_range: usize,
    mem_page_size: usize,
    mem_block_num: usize,
    mem_avail: usize,
    frame_array: Vec<PageState>,
    free_list: Vec<BTreeSet<usize>>,
    // list[0] => 2^0
    // list[1] => 2^1
    list_size: usize,
}

// free list and array management
impl MemoryManager {
    // get the least free block, return the page number and block size
    fn get_least_free(&mut self, target_bs: usize) -> Option<(usize, usize)> {
        // find least larger block
        let mut free_exp: usize = 0;
        while free_exp < self.list_size {
            let cur_bs: usize = 1 << free_exp;
            if target_bs <= cur_bs && self.free_list[free_exp].len() != 0 {
                // we find a block with size cur_bs
                if let Some(&free_pn) = self.free_list[free_exp].iter().next() {
                    // println!("get free block: {} {}", free_pn, free_exp);
                    return Some((free_pn, cur_bs));
                }
            }
            free_exp += 1;
        }
        panic!("Out of memory");
    }
    // push a block to free list
    fn push_free_list(&mut self, pn: usize, size: usize) {
        self.free_list[size.ilog2() as usize].insert(pn);
    }

    fn remove_free_list(&mut self, pn: usize, size: usize) {
        self.free_list[size.ilog2() as usize].remove(&pn);
    }

    fn get_block_size(&self, pn: usize) -> usize {
        match self.frame_array[pn] {
            PageState::Free(bs) => bs,
            PageState::Used(bs) => bs,
            _ => panic!("Buddy System: The block is buddy"),
        }
    }

    fn push_free(&mut self, pn: usize, size: usize) {
        self.set_block_state(pn, PageState::Free(size));
        self.push_free_list(pn, size);
    }

    // maintain the array
    fn set_block_state(&mut self, pn: usize, state: PageState) {
        self.frame_array[pn] = state;
        match self.frame_array[pn] {
            PageState::Free(bs) => {
                for i in pn + 1..pn + bs {
                    self.frame_array[i] = PageState::Buddy(pn);
                }
            }
            PageState::Used(bs) => {
                for i in pn + 1..pn + bs {
                    self.frame_array[i] = PageState::Buddy(pn);
                }
            }
            _ => {}
        }
    }
    // mark a block as used, and pop from free list
    fn mark_block_used(&mut self, pn: usize) {
        let size = self.get_block_size(pn);
        self.set_block_state(pn, PageState::Used(size));
        self.remove_free_list(pn, size);
    }

    fn get_block_state(&self, pn: usize) -> &PageState {
        &self.frame_array[pn]
    }
}

// split and merge
impl MemoryManager {
    // split a block recursively, and return the best fit page number
    // note: the block be split should not be in the free list
    fn split_block_by_size(&mut self, target_size: usize, current_pn: usize) -> usize {
        let current_size = self.get_block_size(current_pn);
        if target_size <= current_size / 2 {
            println!(
                "MemoryManager: Split block: page number:{}, current size:{}, target size:{}",
                current_pn, current_size, target_size
            );
            // we can split the block
            let new_pn = self.split_block(current_pn);
            self.split_block_by_size(target_size, new_pn.0)
        } else {
            // we cannot devide the block anymore
            // we should return the current block
            // and push the rest to free list
            current_pn
        }
    }
    // return pair of page number
    fn split_block(&mut self, current_pn: usize) -> (usize, usize) {
        match self.frame_array[current_pn] {
            PageState::Free(current_size) => {
                let new_size = current_size / 2;
                let new_pn = (current_pn, current_pn + new_size);
                self.push_free(new_pn.1, new_size);
                self.push_free(new_pn.0, new_size);
                // delete the current block from free list
                self.remove_free_list(current_pn, current_size);
                return new_pn;
            }
            _ => {
                panic!("Memory Manager: Splitting a non free block.");
            }
        }
    }

    fn merge_block(&mut self, pn: usize) {
        // check if the buddy is free
        // if so, merge the block and push to free list
        let current_pn = pn;
        let current_size = self.get_block_size(current_pn);

        // find the corresponding buddy
        let buddy_pn = if current_pn % (current_size * 2) == 0 {
            current_pn + current_size
        } else {
            current_pn - current_size
        };
        if buddy_pn >= self.mem_block_num {
            return;
        }
        // check if the buddy is free
        // if so, merge the block and push to free list
        match self.frame_array[buddy_pn] {
            PageState::Free(bs) => {
                // merge the block
                println!("merge block: {} {}", current_pn, buddy_pn);
                let new_pn = if current_pn < buddy_pn {
                    current_pn
                } else {
                    buddy_pn
                };
                let new_size = current_size * 2;
                self.push_free(new_pn, new_size);
                // remove the blocks from free list
                self.remove_free_list(buddy_pn, bs);
                self.remove_free_list(current_pn, bs);
                // merge the new block
                self.merge_block(new_pn);
            }
            PageState::Used(_bs) => {
                // println!("merge block: {} {} buddy is used", current_pn, buddy_pn);
            }
            PageState::Buddy(_pn) => {
                // println!("merge block: {} {} buddy is buddy", current_pn, buddy_pn);
            }
            _ => return,
        };
    }

    fn reserve_page(&mut self, page_number: usize) {
        // check if the page is free
        // split it into single page so we can reserve it
        loop {
            match self.frame_array[page_number] {
                PageState::Free(1) => {
                    break;
                }
                PageState::Free(bs) => {
                    self.split_block(page_number);
                }
                PageState::Buddy(pn) => {
                    self.split_block(pn);
                }
                _ => {
                    panic!("Reserve a non free block");
                }
            };
        }
        self.set_block_state(page_number, PageState::Rsv);
        self.free_list[0].remove(&page_number);
    }
}

// public interface
impl MemoryManager {
    pub fn new(mem_start: usize, mem_end: usize, page_size: usize) -> Self {
        // start and end should be page aligned
        let page_num = (mem_end - mem_start) / page_size;
        // a naive way to calculate the page level
        let page_level = (page_num.ilog2() + 1) as usize;
        let mut manager = MemoryManager {
            mem_addr_start: mem_start,
            mem_addr_end: mem_end,
            mem_addr_range: mem_end - mem_start,
            mem_page_size: page_size,
            mem_block_num: page_num,
            mem_avail: page_num,
            frame_array: vec![PageState::Free(0); page_num],
            free_list: vec![BTreeSet::new(); page_level],
            list_size: page_level,
        };
        // push the whole memory to the free list
        //  find the largest 2^n block
        let mut remain = page_num;
        let mut page_number = 0;
        while remain > 0{
            let block_size = 1 << remain.ilog2();
            manager.push_free(page_number, block_size);
            page_number += block_size;
            remain -= block_size;
        }

        //  manager.push_free(0, page_num);
        //  // initialize the frame array, other pages are buddy of the first page
        //  for i in 1..page_num {
        //     manager.frame_array[i] = PageState::Buddy(0);
        // }

        manager
    }

    // allocate a block of continuous pages, return the start page number
    pub fn get_free_block(&mut self, p_size: usize) -> usize {
        match self.get_least_free(p_size) {
            Some((pn, _bs)) => {
                // we find a block, but it may be larger than we need
                let pn = self.split_block_by_size(p_size, pn);
                self.mark_block_used(pn);
                return pn;
            }
            None => {
                // we cannot find a block
                panic!("Out of memory");
            }
        }
    }

    // alloc in address form
    pub fn alloc(&mut self, size: usize) -> *mut u8 {
        let page_num = size;
        let pn: usize = self.get_free_block(page_num);
        let addr: usize = self.mem_addr_start + pn * self.mem_page_size;
        addr as *mut u8
    }

    // dealloc in address form
    pub fn dealloc(&mut self, addr: *mut u8) {
        let pn = (addr as usize - self.mem_addr_start) / self.mem_page_size;
        self.free_page(pn);
    }

    pub fn show(&self) {
        // println!("{:?}", self.frame_array);

        for i in 0..self.list_size {
            println!("Free list {}: {:?}", i, self.free_list[i]);
        }
    }

    pub fn reserve_addr(&mut self, p_start_addr: usize, p_end_addr: usize) {
        let pn = (p_start_addr - self.mem_addr_start) / self.mem_page_size;
        let size = (p_end_addr - p_start_addr) / self.mem_page_size;
        for i in pn..pn + size {
            self.reserve_page(i);
        }
    }

    pub fn free_page(&mut self, page_number: usize) {
        self.push_free(page_number, self.get_block_size(page_number));
        self.merge_block(page_number);
    }
}
