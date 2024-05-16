/*
* Usage:
*    unsafe {
*        println!(
*            "{:#08x}",
*            &memory::__phys_dram_start_addr as *const usize as usize
*        );
*        println!(
*            "{:#08x}",
*            &memory::__phys_binary_load_addr as *const usize as usize
*        );
*        println!("{:#08x}", &memory::__bss_begin as *const usize as usize);
*        println!("{:#08x}", &memory::__bss_end as *const usize as usize);
*    }
*/

use self::{buddy_page_allocator::BuddyPageAllocator, slab_allocator::SlabAllocator};

extern "C" {
    pub static __phys_dram_start_addr: usize;
    pub static __phys_binary_load_addr: usize;
    pub static __bss_begin: usize;
    pub static __bss_end: usize;
    pub static __heap_begin: usize;
    pub static __heap_end: usize;
}

pub static mut DEVICETREE_START_ADDR: usize = 0;
pub static mut CPIO_START_ADDR: usize = 0;
pub static mut CPIO_END_ADDR: usize = 0;
pub const PAGE_SIZE: usize = 4096;

#[inline(always)]
pub const fn round_up_with(v: usize, s: usize) -> usize {
    (v + s - 1) & !(s - 1)
}

#[inline(always)]
pub const fn round_down_with(v: usize, s: usize) -> usize {
    v & !(s - 1)
}

#[inline(always)]
pub const fn round_up(addr: usize) -> usize {
    round_up_with(addr, PAGE_SIZE)
}

#[inline(always)]
pub const fn round_down(addr: usize) -> usize {
    round_down_with(addr, PAGE_SIZE)
}

#[inline(always)]
pub fn phys_dram_start_addr() -> usize {
    unsafe { &__phys_dram_start_addr as *const usize as usize }
}

#[inline(always)]
pub fn phys_binary_load_addr() -> usize {
    unsafe { &__phys_binary_load_addr as *const usize as usize }
}

#[inline(always)]
unsafe fn heap_start_addr() -> usize {
    &__heap_begin as *const usize as usize
}

#[inline(always)]
unsafe fn heap_end_addr() -> usize {
    &__heap_end as *const usize as usize
}

pub unsafe fn init_allocator() {
    // initialize buddy memory allocator first
    let mut devicetree = devicetree::FlattenedDevicetree::from_memory(DEVICETREE_START_ADDR);
    devicetree
        .traverse(&move |device_name, property_name, property_value| {
            if device_name == "memory@0" && property_name == "reg" {
                let memory_end_addr = u64::from_be_bytes(property_value.try_into().unwrap());
                BUDDY_PAGE_ALLOCATOR.init(0, memory_end_addr as usize)?;
            }
            Ok(())
        })
        .unwrap();
    // for device
    devicetree
        .traverse_reserved_memory(&|start, size| {
            BUDDY_PAGE_ALLOCATOR
                .reserve_memory(start as usize, (start + size) as usize)
                .unwrap();
            Ok(())
        })
        .unwrap();
    // kernel / stack / heap...
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(phys_dram_start_addr(), heap_end_addr())
        .unwrap();
    // CPIO archive
    devicetree
        .traverse(&|device_name, property_name, property_value| {
            if property_name == "linux,initrd-start" {
                CPIO_START_ADDR = u32::from_be_bytes(property_value.try_into().unwrap()) as usize;
                return Ok(());
            }
            Ok(())
        })
        .unwrap();
    devicetree
        .traverse(&|device_name, property_name, property_value| {
            if property_name == "linux,initrd-end" {
                CPIO_END_ADDR = u32::from_be_bytes(property_value.try_into().unwrap()) as usize;
                return Ok(());
            }
            Ok(())
        })
        .unwrap();
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(CPIO_START_ADDR, round_up(CPIO_END_ADDR))
        .unwrap();
    // device tree
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(
            round_down(DEVICETREE_START_ADDR),
            round_up(DEVICETREE_START_ADDR + devicetree.header().total_size() as usize),
        )
        .unwrap();
    // simple allocator
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(heap_start_addr(), heap_end_addr())
        .unwrap();
}

mod buddy_page_allocator;
mod slab_allocator;

static BUDDY_PAGE_ALLOCATOR: BuddyPageAllocator = unsafe { BuddyPageAllocator::new() };

#[global_allocator]
static SLAB_ALLOCATOR: SlabAllocator = unsafe { SlabAllocator::new(&BUDDY_PAGE_ALLOCATOR) };
