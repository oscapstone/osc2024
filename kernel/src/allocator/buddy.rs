use super::bump;
use alloc::collections::BTreeSet;
use core::alloc::{GlobalAlloc, Layout};
use stdio::println;

#[derive(Clone, Copy, Eq, PartialEq, Debug)]
enum BuddyState {
    Head(usize),
    Owned(usize),
    Allocated,
}

const MEMORY_START: u32 = 0x1000_0000;
// const MEMORY_END: u32 = 0x2000_0000;
const MEMORY_END: u32 = 0x1fff_ffff;
const FRAME_SIZE: usize = 0x1000;

const fn frame_count() -> usize {
    (MEMORY_END - MEMORY_START) as usize / FRAME_SIZE
}

#[derive(Clone, Copy)]
struct Frame {
    state: BuddyState,
}

const LAYER_COUNT: usize = 12;

pub struct BuddyAllocator {
    frames: [Frame; frame_count()],
    free_list: [BTreeSet<usize, bump::BumpAllocator>; LAYER_COUNT],
}

impl BuddyAllocator {
    pub const fn new() -> Self {
        const EMPTY: BTreeSet<usize, bump::BumpAllocator> = BTreeSet::new_in(bump::BumpAllocator);
        Self {
            frames: [Frame {
                state: BuddyState::Allocated,
            }; frame_count()],
            free_list: [EMPTY; LAYER_COUNT],
        }
    }
    pub unsafe fn init(&mut self) {
        println!("Initializing buddy allocator");
        println!("Frame count: {}", frame_count());
        assert!(
            MEMORY_START % (1 << LAYER_COUNT) as u32 == 0,
            "Memory start 0x{:x} must be aligned to layer count {}",
            MEMORY_START,
            LAYER_COUNT
        );
        for idx in 0..frame_count() {
            if let BuddyState::Owned(_) = BUDDY_SYSTEM.frames[idx].state {
                continue;
            }
            for layer in (0..LAYER_COUNT).rev() {
                if idx % (1 << layer) == 0 && (idx + (1 << layer) <= frame_count()) {
                    BUDDY_SYSTEM.frames[idx].state = BuddyState::Head(layer);
                    BUDDY_SYSTEM.free_list[layer].insert(idx);
                    for i in 0..(1 << layer) {
                        BUDDY_SYSTEM.frames[idx + i].state = BuddyState::Owned(i);
                    }
                    break;
                }
            }
        }
        BUDDY_SYSTEM.print();
    }
    pub unsafe fn print(&self) {
        println!("Free list:");
        for i in 0..LAYER_COUNT {
            println!(
                "Layer {} ({} byte): {:?}",
                i,
                (1 << i) * FRAME_SIZE,
                BUDDY_SYSTEM.free_list[i]
            );
        }
    }

    fn faddr(&self, idx: usize) -> u32 {
        MEMORY_START + (idx * FRAME_SIZE) as u32
    }

    pub unsafe fn get_by_layout(&mut self, size: usize, align: usize) -> Option<usize> {
        let mut layer = align.trailing_zeros() as usize;
        while (1 << layer) * FRAME_SIZE < size {
            layer += 1;
        }
        if layer < LAYER_COUNT {
            BUDDY_SYSTEM.get_by_layer(layer)
        } else {
            None
        }
    }

    unsafe fn get_by_layer(&mut self, layer: usize) -> Option<usize> {
        // println!("Getting layer: {}", layer);
        assert!(layer < LAYER_COUNT);
        match BUDDY_SYSTEM.free_list[layer].first() {
            Some(idx) => {
                // debug!("Got frame at idx: {}", idx);
                let idx = *idx;
                assert!(BUDDY_SYSTEM.free_list[layer].remove(&idx));
                for i in 0..(1 << layer) {
                    BUDDY_SYSTEM.frames[idx + i].state = BuddyState::Allocated;
                }
                // println!("Got frame at idx: {} addr: 0x{:x}", idx, addr);
                Some(idx)
            }
            None => {
                if layer == LAYER_COUNT - 1 {
                    None
                } else {
                    let idx = BUDDY_SYSTEM.get_by_layer(layer + 1)?;
                    let buddy = idx ^ (1 << layer);
                    BUDDY_SYSTEM.frames[idx].state = BuddyState::Head(layer);
                    for i in 1..(1 << layer) {
                        BUDDY_SYSTEM.frames[idx + i].state = BuddyState::Owned(idx);
                    }
                    BUDDY_SYSTEM.frames[buddy].state = BuddyState::Head(layer);
                    for i in 1..(1 << layer) {
                        BUDDY_SYSTEM.frames[buddy + i].state = BuddyState::Owned(buddy);
                    }
                    BUDDY_SYSTEM.free_list[layer].insert(idx);
                    BUDDY_SYSTEM.free_list[layer].insert(buddy);
                    println!("Split frame {} into {} and {}", idx, idx, buddy);
                    BUDDY_SYSTEM.get_by_layer(layer)
                }
            }
        }
    }

    unsafe fn free_by_idx(&mut self, mut idx: usize) {
        let mut layer = 0;
        loop {
            let buddy = idx ^ (1 << layer);
            if buddy >= frame_count() {
                println!("Buddy out of range");
                break;
            }
            match BUDDY_SYSTEM.frames[buddy].state {
                BuddyState::Head(l) => {
                    if l == layer {
                        BUDDY_SYSTEM.free_list[layer].remove(&buddy);
                        println!("Merged frame {} and {}", idx, buddy);
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
        println!("New free idx: {}", idx);
    }
}

unsafe impl GlobalAlloc for BuddyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();
        let align = layout.align();
        let ret = BUDDY_SYSTEM.get_by_layout(size, align);
        if let Some(idx) = ret {
            let addr = BUDDY_SYSTEM.faddr(idx);
            println!("Allocated frame {} {:?} at 0x{:x}", idx, layout, addr);
            assert!(addr % align as u32 == 0);
            addr as *mut u8
        } else {
            BUDDY_SYSTEM.print();
            panic!("Out of memory");
        }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let idx = (ptr as usize - MEMORY_START as usize) / FRAME_SIZE;
        println!(
            "Deallocating frame {} {:?} at 0x{:x}",
            idx, layout, ptr as usize
        );
        BUDDY_SYSTEM.free_by_idx(idx);
    }
}

pub static mut BUDDY_SYSTEM: BuddyAllocator = BuddyAllocator::new();
