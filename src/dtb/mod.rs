mod dt;
mod fdt;
mod mem_rsvmap;
mod strings;

use stdio;

pub fn load_dtb() {
    stdio::println("Loading DTB...");
    let dtb_addr = 0x50000 as *const u8;
    let dtb_addr = unsafe { core::ptr::read_volatile(dtb_addr as *const u32) };
    let header = fdt::FdtHeader::load(dtb_addr);
    let mem_rsvmap_addr = dtb_addr + header.off_mem_rsvmap;
    let _mem_rsvmap = mem_rsvmap::MemRsvMap::load(mem_rsvmap_addr);
    let strings_addr = dtb_addr + header.off_dt_strings;
    let strings = strings::StringMap::load(strings_addr);
    let dt_struct_addr = dtb_addr + header.off_dt_struct;
    let dt = dt::Dt::load(dt_struct_addr, &strings);

    let model = dt.get("linux,initrd-start");
    stdio::print("initrd_start: ");
    if let Some(model) = model {
        model.print();
    } else {
        stdio::print("N/A");
    }
}
