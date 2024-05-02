mod dt;
mod fdt;
mod mem_rsvmap;
mod strings;
mod utils;

use stdio::println;

fn load_dtb() -> dt::Dt {
    let dtb_addr = 0x6f000 as *const u8;
    println!("DTB address: {:p}", dtb_addr);

    let dtb_addr = unsafe { core::ptr::read_volatile(dtb_addr as *const u32) };

    println!("DTB address: {:#x}", dtb_addr);
    let header = fdt::FdtHeader::load(dtb_addr);
    assert!(header.magic == 0xd00dfeed);

    let mem_rsvmap_addr = dtb_addr + header.off_mem_rsvmap;
    let _mem_rsvmap = mem_rsvmap::MemRsvMap::load(mem_rsvmap_addr);
    stdio::println!("{:?}", _mem_rsvmap);

    let strings_addr = dtb_addr + header.off_dt_strings;
    let strings = strings::StringMap::load(strings_addr);
    let dt_struct_addr = dtb_addr + header.off_dt_struct;
    let dt = dt::Dt::load(dt_struct_addr, &strings);
    dt
}

pub fn get_initrd_start() -> Option<u32> {
    let dt = load_dtb();
    let node = dt.get("linux,initrd-start");
    match node {
        Some(node) => match node.value {
            dt::PropValue::Integer(value) => Some(value),
            _ => None,
        },
        None => None,
    }
}
