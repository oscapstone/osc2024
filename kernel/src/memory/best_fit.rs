
use alloc::collections::{btree_map::BTreeMap, linked_list::Cursor};
use core::{alloc::Layout, borrow::Borrow, iter};

use crate::println;

#[derive(Clone, Debug)]
enum BlockState {
    Free(usize),
    Used(usize),
}

pub struct BestFitAllocator {
    mem_start: usize,
    mem_end: usize,
    addr_map: BTreeMap<usize, BlockState>, // addr -> size
}

impl BestFitAllocator {
    pub fn new(mem_start: usize, mem_end: usize) -> Self {
        let mut addr_map = BTreeMap::new();
        addr_map.insert(mem_start, BlockState::Free(mem_end - mem_start));
        Self {
            mem_start,
            mem_end,
            addr_map: addr_map,
        }
    }

    pub fn alloc(&mut self, layout: Layout) ->Option<*mut u8> {
        let size = layout.size();
        let align = layout.align();
        let mut addr = self.mem_start;
        for (block_addr, block_state) in &self.addr_map {
            match block_state {
                BlockState::Free(block_size) => {
                    let block_start_addr = *block_addr;
                    let block_end_addr = block_start_addr + block_size;
                    let block_align = block_start_addr + align - 1 & !(align - 1);
                    if block_align < block_end_addr && block_end_addr - block_align >= size {
                        // can allocate
                        // note insert will overwrite the block with same address
                        let new_block_size = size;
                        let new_block_addr = block_align;
                        let new_block_end_addr = new_block_addr + new_block_size;
                        // check if we need to split the block
                        if block_start_addr != new_block_addr {
                            let new_block_size = new_block_addr - block_start_addr;
                            self.addr_map
                                .insert(block_start_addr, BlockState::Free(new_block_size));
                        }
                        if block_end_addr != new_block_end_addr {
                            let new_block_size = block_end_addr - new_block_end_addr;
                            self.addr_map
                                .insert(block_align + size, BlockState::Free(new_block_size));
                        }
                        self.addr_map.insert(block_align, BlockState::Used(size));

                        return Some(block_align as *mut u8);
                    }
                }
                _ => {}
            }
        }
        None
    }

    pub fn dealloc(&mut self, ptr: *mut u8) {
        // find the block
        let addr = ptr as usize;
        // find the block and free it
        let mut cur_block_size = 0;
        self.addr_map
            .entry(addr)
            .and_modify(|block_state| match block_state.clone() {
                BlockState::Used(block_size) => {
                    *block_state = BlockState::Free(block_size);
                    cur_block_size = block_size;
                }
                _ => {
                    panic!("Double free detected!");
                }
            });
        if let Some((block_addr, block_state)) = self.addr_map.range((addr + 1)..).next() {
            match *block_state {
                // check if we can merge with the next block
                BlockState::Free(next_block_size) => {
                    // merge with the next block
                    // remove the next block
                    self.addr_map.remove(&block_addr.clone());
                    let new_block_size = next_block_size + cur_block_size;
                    // note insert will overwrite the block with same address
                    self.addr_map.insert(addr, BlockState::Free(new_block_size));
                }
                _ => {}
            }
        }

        if let Some(block_state) = self.addr_map.get(&addr) {
            match block_state {
                BlockState::Free(block_size) => {
                    cur_block_size = *block_size;
                }
                _ => {}
            }
        }

        // check if we can merge with the previous block
        if let Some((prev_block_addr, block_state)) = self.addr_map.range(..addr).next_back() {
            match *block_state {
                BlockState::Free(block_size) => {
                    // merge with the previous block
                    let prev_block_addr = prev_block_addr.clone();
                    let new_block_size = block_size + (cur_block_size as usize);
                    // remove the current block
                    self.addr_map.remove(&addr);
                    self.addr_map
                        .insert(prev_block_addr, BlockState::Free(new_block_size));
                }
                _ => {}
            }
        }
    }

    pub fn show(&self) {
        println!("{:?}", self.addr_map);
    }
}
