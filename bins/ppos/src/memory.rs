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

use alloc::boxed::Box;
use library::println;

use self::{buddy_page_allocator::BuddyPageAllocator, slab_allocator::SlabAllocator};

extern "C" {
    static __bss_begin: usize;
    static __bss_end: usize;
    static __heap_begin: usize;
    static __heap_end: usize;
    static __virtual_kernel_space_addr: usize;
    static __virtual_kernel_start_addr: usize;
}

pub static mut DEVICETREE_START_ADDR: usize = 0;
pub static mut CPIO_START_ADDR: usize = 0;
pub static mut CPIO_END_ADDR: usize = 0;
pub const PAGE_SIZE: usize = 4096;

#[derive(Debug)]
#[repr(C)]
pub struct AllocatedMemory {
    inner: Box<[u8]>,
}

impl AllocatedMemory {
    #[inline(always)]
    pub const fn new(memory: Box<[u8]>) -> Self {
        Self { inner: memory }
    }

    #[inline(always)]
    pub fn top(&self) -> *const u8 {
        self.inner.as_ptr()
    }

    #[inline(always)]
    pub fn bottom(&self) -> *const u8 {
        (self.inner.as_ptr() as usize + self.inner.len()) as *const u8
    }
}

impl core::ops::Deref for AllocatedMemory {
    type Target = Box<[u8]>;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

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
pub const fn phys_to_virt(addr: usize) -> usize {
    addr + virtual_kernel_space_addr()
}

#[inline(always)]
pub const fn virt_to_phys(addr: usize) -> usize {
    addr - virtual_kernel_space_addr()
}

#[inline(always)]
pub const fn phys_dram_start_addr() -> usize {
    0
}

#[inline(always)]
pub const fn phys_binary_load_addr() -> usize {
    0x80000
}

#[inline(always)]
pub const fn virtual_kernel_space_addr() -> usize {
    0xffff_0000_0000_0000
}

#[inline(always)]
pub const fn virtual_kernel_start_addr() -> usize {
    phys_to_virt(phys_binary_load_addr())
}

#[inline(always)]
pub fn heap_start_addr() -> usize {
    unsafe { &__heap_begin as *const usize as usize }
}

#[inline(always)]
pub fn heap_end_addr() -> usize {
    unsafe { &__heap_end as *const usize as usize }
}

pub unsafe fn init_allocator() {
    // initialize buddy memory allocator first
    let mut devicetree =
        devicetree::FlattenedDevicetree::from_memory(phys_to_virt(DEVICETREE_START_ADDR));
    devicetree
        .traverse(&move |device_name, property_name, property_value| {
            if device_name == "memory@0" && property_name == "reg" {
                let memory_end_addr = u64::from_be_bytes(property_value.try_into().unwrap());
                BUDDY_PAGE_ALLOCATOR
                    .init(phys_to_virt(0), phys_to_virt(memory_end_addr as usize))?;
            }
            Ok(())
        })
        .unwrap();
    // for device
    devicetree
        .traverse_reserved_memory(&|start, size| {
            BUDDY_PAGE_ALLOCATOR
                .reserve_memory(
                    phys_to_virt(start as usize),
                    phys_to_virt((start + size) as usize),
                )
                .unwrap();
            Ok(())
        })
        .unwrap();
    // kernel / stack / heap...
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(virtual_kernel_space_addr(), heap_end_addr())
        .unwrap();
    // CPIO archive
    devicetree
        .traverse(&|device_name, property_name, property_value| {
            if property_name == "linux,initrd-start" {
                CPIO_START_ADDR = u32::from_be_bytes(property_value.try_into().unwrap()) as usize;
                return Ok(());
            } else if property_name == "linux,initrd-end" {
                CPIO_END_ADDR = u32::from_be_bytes(property_value.try_into().unwrap()) as usize;
                return Ok(());
            }
            Ok(())
        })
        .unwrap();
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(
            phys_to_virt(CPIO_START_ADDR),
            phys_to_virt(round_up(CPIO_END_ADDR)),
        )
        .unwrap();
    // device tree
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(
            phys_to_virt(round_down(DEVICETREE_START_ADDR)),
            phys_to_virt(round_up(
                DEVICETREE_START_ADDR + devicetree.header().total_size() as usize,
            )),
        )
        .unwrap();
    // simple allocator
    BUDDY_PAGE_ALLOCATOR
        .reserve_memory(heap_start_addr(), heap_end_addr())
        .unwrap();
}

mod buddy_page_allocator;
mod slab_allocator;

pub mod paging;

static BUDDY_PAGE_ALLOCATOR: BuddyPageAllocator = unsafe { BuddyPageAllocator::new() };

#[global_allocator]
static SLAB_ALLOCATOR: SlabAllocator = unsafe { SlabAllocator::new(&BUDDY_PAGE_ALLOCATOR) };

pub fn print_memory_info() {
    println!("{}", BUDDY_PAGE_ALLOCATOR);
}
