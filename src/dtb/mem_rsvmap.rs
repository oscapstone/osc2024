use alloc::vec::Vec;
use stdio;

#[repr(C)]
pub struct MemRsvMap {
    pub mem_rsv_map: Vec<MemRsv>,
}

impl MemRsvMap {
    pub fn load(mem_rsvmap_addr: u32) -> MemRsvMap {
        let mut mem_rsv_map = Vec::new();
        let mut mem_rsv_addr = mem_rsvmap_addr;
        loop {
            let mem_rsv = MemRsv::load(mem_rsv_addr);
            if mem_rsv.addr == 0 && mem_rsv.size == 0 {
                break;
            }
            mem_rsv_map.push(mem_rsv);
            mem_rsv_addr += core::mem::size_of::<MemRsv>() as u32;
        }
        MemRsvMap { mem_rsv_map }
    }
    pub fn print(&self) {
        for mem_rsv in self.mem_rsv_map.iter() {
            mem_rsv.print();
        }
    }
}

#[repr(C, packed)]
#[derive(Copy, Clone)]
struct MemRsv {
    pub addr: u64,
    pub size: u64,
}

impl MemRsv {
    pub fn load(mem_rsv_addr: u32) -> MemRsv {
        let mem_rsv = unsafe { &*(mem_rsv_addr as *const MemRsv) };
        MemRsv {
            addr: mem_rsv.addr.swap_bytes() as u64,
            size: mem_rsv.size.swap_bytes() as u64,
        }
    }
    pub fn print(&self) {
        stdio::print("Addr: ");
        stdio::print_u64(self.addr);
        stdio::println("");
        stdio::print("Size: ");
        stdio::print_u64(self.size);
        stdio::println("");
    }
}
