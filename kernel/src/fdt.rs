// A parser parse device tree;

use alloc::vec::Vec;

#[repr(C, packed)]
pub struct FdtHeader {
    pub magic: u32,
    pub totalsize: u32,
    pub off_dt_struct: u32,
    pub off_dt_strings: u32,
    pub off_mem_rsvmap: u32,
    pub version: u32,
    pub last_comp_version: u32,
    pub boot_cpuid_phys: u32,
    pub size_dt_strings: u32,
    pub size_dt_struct: u32,
}

pub struct FdtReserveEntry {
    address: u64,
    size: u64,
}
impl FdtReserveEntry {
    pub fn get_address(&self) -> u64 {
        self.address
    }
    pub fn get_size(&self) -> u64 {
        self.size
    }
}

#[repr(C, packed)]
struct FdtProp {
    len: u32,
    nameoff: u32,
}

pub struct DtbParser {
    pub header: *mut FdtHeader,
    pub magic: u32,
    pub totalsize: u32,
    pub off_dt_struct: u32,
    pub off_dt_strings: u32,
    pub off_mem_rsvmap: u32,
    pub version: u32,
    pub last_comp_version: u32,
    pub boot_cpuid_phys: u32,
    pub size_dt_strings: u32,
    pub size_dt_struct: u32,
    pub fdt_reserve_entries: Vec<FdtReserveEntry>,
}

impl DtbParser {
    pub fn new(dtb: *const u8) -> DtbParser {
        let h: *mut FdtHeader = dtb as *mut FdtHeader;
        unsafe {
            DtbParser {
                header: dtb as *mut FdtHeader,
                magic: (*h).magic.swap_bytes(),
                totalsize: (*h).totalsize.swap_bytes(),
                off_dt_struct: (*h).off_dt_struct.swap_bytes(),
                off_dt_strings: (*h).off_dt_strings.swap_bytes(),
                off_mem_rsvmap: (*h).off_mem_rsvmap.swap_bytes(),
                version: (*h).version.swap_bytes(),
                last_comp_version: (*h).last_comp_version.swap_bytes(),
                boot_cpuid_phys: (*h).boot_cpuid_phys.swap_bytes(),
                size_dt_strings: (*h).size_dt_strings.swap_bytes(),
                size_dt_struct: (*h).size_dt_struct.swap_bytes(),
                fdt_reserve_entries: mem_rsvmap_paser(dtb, (*h).off_mem_rsvmap),
            }
        }
    }

    pub fn get_fdt_reserve_entries(&self) -> &Vec<FdtReserveEntry> {
        &self.fdt_reserve_entries
    }
    // parse structure block
    pub fn parse_struct_block(&self) {
        let mut offset = self.off_dt_struct as usize;
        let h: *mut u8 = self.header as *mut u8;
        unsafe {
            loop {
                crate::println!("Offset: {:#x}", offset);
                let token = (*(h.add(offset) as *const u32)).swap_bytes();
                match token {
                    FDT_BEGIN_NODE => {
                        crate::println!("Node: ");
                        offset += 4;
                        let name = c_str_to_str(h.add(offset));
                        crate::println!("\tName: /{}", name);
                        crate::println!("\tName Length: {}", name.len());
                        offset += name.len() + 1;
                    }
                    FDT_END_NODE => {
                        crate::println!("End Node");
                        offset += 4;
                    }
                    FDT_PROP => {
                        offset += 4;
                        let prop = h.add(offset as usize) as *const FdtProp;
                        let len = (*prop).len.swap_bytes() as usize;
                        let nameoff = (*prop).nameoff.swap_bytes() as usize;
                        let name = h.add(nameoff + self.off_dt_strings as usize);
                        // print name of property
                        crate::println!("Property: ");
                        crate::println!("\tLength: {}", len);
                        crate::println!("\tName: {}", c_str_to_str(name));
                        // print value
                        let value = h.add(offset + 8);
                        crate::println!("\tValue:");
                        crate::print!("\t\t");
                        for i in 0..len {
                            crate::print!("{:02x} ", *value.add(i));
                            if i % 16 == 15 {
                                crate::println!();
                                crate::print!("\t\t");
                            }
                        }
                        crate::println!();
                        crate::print!("\t\t");
                        for i in 0..len {
                            let val = *value.add(i);
                            if val >= 32 && val <= 126 {
                                crate::print!("{}", val as char);
                            } else {
                                crate::print!(".");
                            }
                        }
                        crate::println!();
                        offset += 8 + len;
                    }
                    FDT_NOP => {
                        crate::println!("NOP");
                        offset += 4;
                    }
                    FDT_END => {
                        crate::println!("End");
                        break;
                    }
                    _ => {
                        crate::println!("Unknown token: {:#x}", token);
                        break;
                    }
                }
                crate::println!();
                // next token
                offset = (offset + 3) & !3;
            }
        }
    }

    pub fn traverse(&self, callback: fn(&str, *const u8, usize) -> Option<u32>) -> Option<u32> {
        let mut offset = self.off_dt_struct as usize;
        let h: *mut u8 = self.header as *mut u8;
        unsafe {
            loop {
                let token = (*(h.add(offset) as *const u32)).swap_bytes();
                match token {
                    FDT_BEGIN_NODE => {
                        offset += 4;
                        let name = c_str_to_str(h.add(offset));
                        offset += name.len() + 1;
                    }
                    FDT_END_NODE => {
                        offset += 4;
                    }
                    FDT_PROP => {
                        offset += 4;
                        let prop = h.add(offset as usize) as *const FdtProp;
                        let len = (*prop).len.swap_bytes() as usize;
                        let nameoff = (*prop).nameoff.swap_bytes() as usize;
                        let name = h.add(nameoff + self.off_dt_strings as usize);
                        let value = h.add(offset + 8);
                        if let Some(ret) = callback(c_str_to_str(name), value, len) {
                            return Some(ret);
                        }
                        offset += 8 + len;
                    }
                    FDT_NOP => {
                        offset += 4;
                    }
                    FDT_END => {
                        break None;
                    }
                    _ => {
                        crate::println!("Unknown token: {:#x}", token);
                        break None;
                    }
                }
                // next token
                offset = (offset + 3) & !3;
            }
        }
    }

    pub fn get_dtb_size(&self) -> u32 {
        return self.totalsize;
    }
}

fn mem_rsvmap_paser(dtb: *const u8, off_mem_rsvmap: u32) -> Vec<FdtReserveEntry> {
    let h: *mut FdtHeader = dtb as *mut FdtHeader;
    let mut entries = Vec::new();
    let mut offset = off_mem_rsvmap.swap_bytes();
    unsafe {
        loop {
            let address = *(dtb.add(offset as usize) as *const u64);
            let size = (*(dtb.add(offset as usize + 8) as *const u64)).swap_bytes();
            if address == 0 && size == 0 {
                break;
            }
            entries.push(FdtReserveEntry { address, size });
            offset += 16;
        }
    }
    entries
}

const FDT_BEGIN_NODE: u32 = 0x00000001;
const FDT_END_NODE: u32 = 0x00000002;
const FDT_PROP: u32 = 0x00000003;
const FDT_NOP: u32 = 0x00000004;
const FDT_END: u32 = 0x00000009;

fn c_str_to_str(s: *const u8) -> &'static str {
    unsafe { core::str::from_utf8_unchecked(core::ffi::CStr::from_ptr(s as *const i8).to_bytes()) }
}
