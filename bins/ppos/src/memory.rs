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
extern "C" {
    pub static __phys_dram_start_addr: usize;
    pub static __phys_binary_load_addr: usize;
    pub static __bss_begin: usize;
    pub static __bss_end: usize;
    pub static __heap_begin: usize;
    pub static __heap_end: usize;
}

pub static mut DEVICETREE_START_ADDR: usize = 0;

pub mod heap_allocator;
