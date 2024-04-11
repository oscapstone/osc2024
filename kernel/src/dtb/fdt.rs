#[repr(C, packed)]
#[derive(Debug)]
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

impl FdtHeader {
    pub fn load(dtb_addr: u32) -> FdtHeader {
        let header = unsafe { &*(dtb_addr as *const FdtHeader) };
        FdtHeader {
            magic: header.magic.swap_bytes(),
            totalsize: header.totalsize.swap_bytes(),
            off_dt_struct: header.off_dt_struct.swap_bytes(),
            off_dt_strings: header.off_dt_strings.swap_bytes(),
            off_mem_rsvmap: header.off_mem_rsvmap.swap_bytes(),
            version: header.version.swap_bytes(),
            last_comp_version: header.last_comp_version.swap_bytes(),
            boot_cpuid_phys: header.boot_cpuid_phys.swap_bytes(),
            size_dt_strings: header.size_dt_strings.swap_bytes(),
            size_dt_struct: header.size_dt_struct.swap_bytes(),
        }
    }
}
