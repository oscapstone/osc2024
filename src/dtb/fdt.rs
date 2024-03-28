use stdio;

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
    pub fn print(&self) {
        stdio::print("Magic: ");
        stdio::print_u32(self.magic);
        stdio::println("");
        stdio::print("Total size: ");
        stdio::print_u32(self.totalsize);
        stdio::println("");
        stdio::print("Off dt struct: ");
        stdio::print_u32(self.off_dt_struct);
        stdio::println("");
        stdio::print("Off dt strings: ");
        stdio::print_u32(self.off_dt_strings);
        stdio::println("");
        stdio::print("Off mem rsvmap: ");
        stdio::print_u32(self.off_mem_rsvmap);
        stdio::println("");
        stdio::print("Version: ");
        stdio::print_u32(self.version);
        stdio::println("");
        stdio::print("Last comp version: ");
        stdio::print_u32(self.last_comp_version);
        stdio::println("");
        stdio::print("Boot cpuid phys: ");
        stdio::print_u32(self.boot_cpuid_phys);
        stdio::println("");
        stdio::print("Size dt strings: ");
        stdio::print_u32(self.size_dt_strings);
        stdio::println("");
        stdio::print("Size dt struct: ");
        stdio::print_u32(self.size_dt_struct);
        stdio::println("");
    }
}
