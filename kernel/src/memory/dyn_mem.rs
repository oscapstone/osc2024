
use alloc::vec::Vec;
use alloc::vec;
use core::alloc::{Layout};

const BITMAP_ELEMENT_SIZE: usize = 64;
pub struct Bitmap {
    size: usize,
    data: Vec<u64>,
}

impl Bitmap {
    pub fn new(size: usize) -> Self {
        Bitmap {
            size,
            data: vec![0; (size + BITMAP_ELEMENT_SIZE - 1) / BITMAP_ELEMENT_SIZE],
        }
    }

    pub fn get(&self, index: usize) -> bool {
        let byte_index = index / BITMAP_ELEMENT_SIZE;
        let bit_index = index % BITMAP_ELEMENT_SIZE;
        self.data[byte_index] & (1 << bit_index) != 0
    }


    pub fn set(&mut self, index: usize) {
        let byte_index = index / BITMAP_ELEMENT_SIZE;
        let bit_index = index % BITMAP_ELEMENT_SIZE;
        self.data[byte_index] |= 1 << bit_index;
    }
}

pub struct DynMemPool {
    block_size: usize, // size of each block
    pool_addr_start: usize,
    pool_addr_end: usize,
    pool_size: usize,
    alloc_bitmap: Bitmap,
}

impl DynMemPool {
    pub fn new(pool_addr_start: usize, pool_addr_end: usize, block_size: usize) -> Self {
        let pool_size = pool_addr_end - pool_addr_start;
        let block_num = pool_size / block_size;
        let mut alloc_bitmap: Bitmap = Bitmap::new(block_num);
        DynMemPool {
            block_size,
            pool_addr_start,
            pool_addr_end,
            pool_size: pool_size,
            alloc_bitmap,
        }
    }

    pub fn alloc(&mut self) -> Option<usize> {
        for i in 0..self.pool_size {
            if !self.alloc_bitmap.get(i) {
                self.alloc_bitmap.set(i);
                return Some(self.pool_addr_start + i * self.block_size);
            }
        }
        None
    }

    pub fn dealloc(&mut self, addr: usize) {
        let index = (addr - self.pool_addr_start) / self.block_size;
        self.alloc_bitmap.set(index);
    }
}
