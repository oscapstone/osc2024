use super::bump::BumpAllocator;
use alloc::{collections::BTreeSet, vec::Vec};
use core::alloc::{GlobalAlloc, Layout};
use stdio::{debug, println};

#[derive(Clone, Copy, Eq, PartialEq, Debug)]
enum BuddyState {
    Head(usize),
    Owned(usize),
    Allocated,
}

use super::config::{FRAME_SIZE, MEMORY_END, MEMORY_START};
const NFRAME: usize = (MEMORY_END - MEMORY_START) as usize / FRAME_SIZE;

#[derive(Clone, Copy)]
struct Frame {
    state: BuddyState,
}

const LAYER_COUNT: usize = 16;

pub struct BuddyAllocator {
    frames: Vec<Frame, BumpAllocator>,
    free_list: [BTreeSet<usize, BumpAllocator>; LAYER_COUNT],
    verbose: bool,
    pub initialized: bool,
}

#[allow(dead_code)]
pub fn toggle_verbose() {
    unsafe {
        BUDDY_SYSTEM.verbose = !BUDDY_SYSTEM.verbose;
    }
    println!("BuddyAllocator verbose: {}", unsafe {
        BUDDY_SYSTEM.verbose
    });
}

impl BuddyAllocator {
    pub const fn new() -> Self {
        const EMPTY: BTreeSet<usize, BumpAllocator> = BTreeSet::new_in(BumpAllocator);
        Self {
            frames: Vec::new_in(BumpAllocator),
            free_list: [EMPTY; LAYER_COUNT],
            verbose: false,
            initialized: false,
        }
    }
    pub unsafe fn init(&mut self) {
        println!("Initializing buddy allocator");
        println!("Frame count: {}", NFRAME);
        assert!(
            MEMORY_START % (1 << LAYER_COUNT) as u32 == 0,
            "Memory start 0x{:x} must be aligned to frame size 0x{:x}",
            MEMORY_START,
            FRAME_SIZE
        );

        for _ in 0..NFRAME {
            self.frames.push(Frame {
                state: BuddyState::Allocated,
            });
        }
        for idx in 0..NFRAME {
            if let BuddyState::Owned(_) = BUDDY_SYSTEM.frames[idx].state {
                continue;
            }
            for layer in (0..LAYER_COUNT).rev() {
                if idx % (1 << layer) == 0 && (idx + (1 << layer) <= NFRAME) {
                    BUDDY_SYSTEM.frames[idx].state = BuddyState::Head(layer);
                    BUDDY_SYSTEM.free_list[layer].insert(idx);
                    for i in 1..(1 << layer) {
                        BUDDY_SYSTEM.frames[idx + i].state = BuddyState::Owned(idx);
                    }
                    println!("Initialized frame {} at layer {}", idx, layer);
                    break;
                }
            }
        }
        println!("Free list: {:?}", BUDDY_SYSTEM.free_list);
        BUDDY_SYSTEM.initialized = true;
    }

    pub unsafe fn print_info(&self) {
        println!("Buddy allocator info:");
        for layer in 0..LAYER_COUNT {
            println!("Layer {}: {:?}", layer, BUDDY_SYSTEM.free_list[layer]);
        }
    }

    fn faddr(&self, idx: usize) -> u32 {
        MEMORY_START + (idx * FRAME_SIZE) as u32
    }

    fn idx(&self, addr: u32) -> usize {
        (addr - MEMORY_START) as usize / FRAME_SIZE
    }

    unsafe fn alloc_frame(&mut self, idx: usize) {
        assert!(idx < NFRAME);
        let layer = match BUDDY_SYSTEM.frames[idx].state {
            BuddyState::Head(l) => l,
            _ => panic!("Invalid state, expected Head"),
        };
        assert!(BUDDY_SYSTEM.free_list[layer].contains(&idx));
        for i in 0..(1 << layer) {
            BUDDY_SYSTEM.frames[idx + i].state = BuddyState::Allocated;
        }
        BUDDY_SYSTEM.free_list[layer].remove(&idx);
    }

    unsafe fn get_by_layout(&mut self, size: usize, align: usize) -> Option<usize> {
        let mut layer = 0;
        if align < FRAME_SIZE {
            layer = 0;
        } else {
            while (1 << layer) < align {
                layer += 1;
            }
        }
        while (1 << layer) * FRAME_SIZE < size {
            layer += 1;
        }
        if layer < LAYER_COUNT {
            BUDDY_SYSTEM.alloc_by_layer(layer)
        } else {
            None
        }
    }

    unsafe fn split_frame(&mut self, idx: usize) {
        // debug!("Splitting frame {}", idx);
        let layer = match BUDDY_SYSTEM.frames[idx].state {
            BuddyState::Head(l) => l,
            _ => panic!("Invalid state, expected Head"),
        };
        assert!(BUDDY_SYSTEM.free_list[layer].contains(&idx));
        let cur = idx;
        BUDDY_SYSTEM.free_list[layer].remove(&cur);
        let buddy = idx ^ (1 << layer - 1);
        BUDDY_SYSTEM.frames[idx].state = BuddyState::Head(layer - 1);
        for i in 1..(1 << layer - 1) {
            BUDDY_SYSTEM.frames[idx + i].state = BuddyState::Owned(idx);
        }
        BUDDY_SYSTEM.frames[buddy].state = BuddyState::Head(layer - 1);
        for i in 1..(1 << layer - 1) {
            BUDDY_SYSTEM.frames[buddy + i].state = BuddyState::Owned(buddy);
        }
        BUDDY_SYSTEM.free_list[layer - 1].insert(idx);
        BUDDY_SYSTEM.free_list[layer - 1].insert(buddy);
        if BUDDY_SYSTEM.verbose {
            println!(
                "Split frame {} from layer {} to layer {}",
                idx,
                layer,
                layer - 1
            );
        }
    }

    unsafe fn get_by_layer(&mut self, layer: usize) -> Option<usize> {
        match BUDDY_SYSTEM.free_list[layer].first() {
            Some(idx) => Some(*idx),
            None => {
                if let Some(idx) = BUDDY_SYSTEM.get_by_layer(layer + 1) {
                    BUDDY_SYSTEM.split_frame(idx);
                    BUDDY_SYSTEM.get_by_layer(layer)
                } else {
                    None
                }
            }
        }
    }

    unsafe fn alloc_by_layer(&mut self, layer: usize) -> Option<usize> {
        if let Some(idx) = BUDDY_SYSTEM.get_by_layer(layer) {
            BUDDY_SYSTEM.alloc_frame(idx);
            Some(idx)
        } else {
            None
        }
    }

    unsafe fn free_by_layout(&mut self, ptr: *mut u8, size: usize, align: usize) {
        let addr = ptr as u32;
        let idx = (addr - MEMORY_START) as usize / FRAME_SIZE;
        let mut layer = 0;
        if align < FRAME_SIZE {
            layer = 0;
        } else {
            while (1 << layer) < align {
                layer += 1;
            }
        }
        while (1 << layer) * FRAME_SIZE < size {
            layer += 1;
        }
        if BUDDY_SYSTEM.verbose {
            println!("Free frame {} at layer {}", idx, layer);
        }
        BUDDY_SYSTEM.free_by_idx(idx, layer);
    }

    pub unsafe fn free_by_idx(&mut self, mut idx: usize, mut layer: usize) {
        loop {
            let buddy = idx ^ (1 << layer);
            if buddy >= NFRAME {
                println!("Buddy out of range");
                break;
            }
            if layer == LAYER_COUNT - 1 {
                break;
            }
            match BUDDY_SYSTEM.frames[buddy].state {
                BuddyState::Head(l) => {
                    if l == layer {
                        BUDDY_SYSTEM.free_list[layer].remove(&buddy);
                        if BUDDY_SYSTEM.verbose {
                            println!("Merged frame {} and {}", idx, buddy);
                        }
                        layer += 1;
                        idx = idx & !((1 << layer) - 1);
                        continue;
                    } else {
                        break;
                    }
                }
                BuddyState::Owned(ord) => {
                    assert!(ord == 0);
                    layer += 1;
                }
                BuddyState::Allocated => {
                    break;
                }
            }
        }
        BUDDY_SYSTEM.frames[idx].state = BuddyState::Head(layer);
        for i in 1..(1 << layer) {
            BUDDY_SYSTEM.frames[idx + i].state = BuddyState::Owned(idx);
        }
        BUDDY_SYSTEM.free_list[layer].insert(idx);
        // println!("New free idx: {}", idx);
    }

    pub unsafe fn get_frame(&mut self, layer: usize, idx: usize) -> Option<usize> {
        assert!(idx < NFRAME, "Frame index out of range");
        assert!(
            BUDDY_SYSTEM.frames[idx].state != BuddyState::Allocated,
            "Frame is allocated"
        );
        // println!("Getting frame {} at layer {}", idx, layer);
        match BUDDY_SYSTEM.frames[idx].state {
            BuddyState::Head(l) => {
                if l == layer {
                    Some(idx)
                } else {
                    BUDDY_SYSTEM.split_frame(idx);
                    BUDDY_SYSTEM.get_frame(layer, idx)
                }
            }
            BuddyState::Owned(h) => {
                assert!(h != idx);
                BUDDY_SYSTEM.split_frame(h);
                BUDDY_SYSTEM.get_frame(layer, idx)
            }
            BuddyState::Allocated => None,
        }
    }

    pub unsafe fn reserve_frame(&mut self, idx: usize) -> bool {
        if idx >= NFRAME {
            debug!("Invalid frame index {} > {}", idx, NFRAME);
        }
        if BUDDY_SYSTEM.frames[idx].state == BuddyState::Allocated {
            println!("Frame {} is already allocated", idx);
            return false;
        }
        BUDDY_SYSTEM.get_frame(0, idx);
        BUDDY_SYSTEM.alloc_frame(idx);
        true
    }

    pub unsafe fn reserve_by_addr_range(&mut self, start: u32, end: u32) -> bool {
        let sidx = BUDDY_SYSTEM.idx(start);
        let eidx = BUDDY_SYSTEM.idx(end);
        debug!(
            "Reserving frames from 0x{:x}({}) to 0x{:x}({})",
            start, sidx, end, eidx
        );
        for idx in sidx..eidx {
            if BUDDY_SYSTEM.verbose {
                println!("Reserving frame {}", idx);
            }
            if !BUDDY_SYSTEM.reserve_frame(idx) {
                return false;
            }
        }
        true
    }
}

unsafe impl GlobalAlloc for BuddyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();
        let align = layout.align();
        let ret = BUDDY_SYSTEM.get_by_layout(size, align);
        if let Some(idx) = ret {
            let addr = BUDDY_SYSTEM.faddr(idx);
            assert!(addr % align as u32 == 0);
            if BUDDY_SYSTEM.verbose {
                debug!(
                    "BuddyAllocator: alloc frame {} at 0x{:x} size {} align {}",
                    idx, addr, size, align
                );
            }
            if BUDDY_SYSTEM.verbose {
                println!("Free list: {:?}", BUDDY_SYSTEM.free_list);
            }
            addr as *mut u8
        } else {
            panic!("Out of memory");
        }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let size = layout.size();
        let align = layout.align();
        BUDDY_SYSTEM.free_by_layout(ptr, size, align);
    }
}

pub static mut BUDDY_SYSTEM: BuddyAllocator = BuddyAllocator::new();
