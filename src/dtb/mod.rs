mod dt;
mod fdt;
mod mem_rsvmap;
mod strings;

use stdio;

pub fn load_dtb() {
    stdio::println("Loading DTB...");
    let dtb_addr = 0x50000 as *const u8;
    let dtb_addr = unsafe { core::ptr::read_volatile(dtb_addr as *const u32) };

    stdio::print("DTB address: ");
    stdio::print_u32(dtb_addr as u32);
    stdio::println("");
    let header = fdt::FdtHeader::load(dtb_addr);
    header.print();
    stdio::println("");

    let mem_rsvmap_addr = dtb_addr + header.off_mem_rsvmap;
    stdio::print("Mem rsvmap address: ");
    stdio::print_u32(mem_rsvmap_addr as u32);
    stdio::println("");
    let mem_rsvmap = mem_rsvmap::MemRsvMap::load(mem_rsvmap_addr);
    mem_rsvmap.print();
    stdio::println("");

    let strings_addr = dtb_addr + header.off_dt_strings;
    stdio::print("Strings address: ");
    stdio::print_u32(strings_addr as u32);
    stdio::println("");

    let strings = strings::StringMap::load(strings_addr);
    // strings.get(0);
    // strings.print();
    stdio::println("");

    let dt_struct_addr = dtb_addr + header.off_dt_struct;
    stdio::print("DT struct address: ");
    stdio::print_u32(dt_struct_addr as u32);
    stdio::println("");
    let dt = dt::Dt::load(dt_struct_addr, &strings);

    stdio::println("Done loading DTB!");
}
