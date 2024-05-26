mod dt;
mod fdt;
mod mem_rsvmap;
mod strings;
mod utils;

use alloc::vec::Vec;
use stdio::println;

const DTB_ADDRERSS: u64 = 0x6_f000;

pub fn load_dtb() -> dt::Dt {
    let (dtb_addr, header) = get_dtb_addr();
    let strings_addr = dtb_addr + header.off_dt_strings;
    let strings = strings::StringMap::load(strings_addr);
    let dt_struct_addr = dtb_addr + header.off_dt_struct;
    let dt = dt::Dt::load(dt_struct_addr, &strings);
    dt
}

pub fn get_dtb_addr() -> (u32, fdt::FdtHeader) {
    let dtb_addr = DTB_ADDRERSS as *const u8;
    println!("DTB address: {:p}", dtb_addr);
    let dtb_addr = unsafe { core::ptr::read_volatile(dtb_addr as *const u32) };
    println!("DTB address: {:#x}", dtb_addr);
    let header = fdt::FdtHeader::load(dtb_addr);
    assert!(header.magic == 0xd00dfeed);
    (dtb_addr, header)
}

pub fn get_initrd_start() -> u32 {
    let dt = load_dtb();
    let node = dt.get("linux,initrd-start");
    match node {
        Some(node) => match node.value {
            dt::PropValue::Integer(value) => value,
            _ => panic!("Invalid initrd-start value"),
        },
        None => panic!("Failed to get initrd start address!"),
    }
}

pub fn get_reserved_memory() -> Vec<(u32, u32)> {
    let (dtb_addr, header) = get_dtb_addr();
    let mem_rsvmap_addr = dtb_addr + header.off_mem_rsvmap;
    let mem_rsvmap = mem_rsvmap::MemRsvMap::load(mem_rsvmap_addr);
    let mut ret = Vec::new();
    for mem_rsv in mem_rsvmap.mem_rsv_map {
        ret.push((mem_rsv.addr as u32, mem_rsv.size as u32));
    }
    ret
}
