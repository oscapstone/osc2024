// https://github.com/repnop/fdt/tree/masterfv
// https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html
// https://www.wowotech.net/device_model/dt-code-file-struct-parse.html
// flatten device tree

const MAGIC_NUMBER: u32 = 0xd00dfeed;

/// Possible errors when attempting to create an `Fdt`
// #[derive(Debug, Clone, Copy, PartialEq)]
// pub enum FdtError {
//     /// The FDT had an invalid magic value
//     BadMagic,
//     /// The given pointer was null
//     BadPtr,
//     /// The slice passed in was too small to fit the given total size of the FDT
//     /// structure
//     BufferTooSmall,
// }

/// A flattened devicetree located somewhere in memory
///
/// Note on `Debug` impl: by default the `Debug` impl of this struct will not
/// print any useful information, if you would like a best-effort tree print
/// which looks similar to `dtc`'s output, enable the `pretty-printing` feature
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FdtHeader {
    magic: u32,
    totalsize: u32,
    off_dt_struct: u32,
    off_dt_strings: u32,
    off_mem_rsvmap: u32,
    version: u32,
    last_comp_version: u32,
    boot_cpuid_phys: u32,
    size_dt_strings: u32,
    size_dt_struct: u32,
}

impl FdtHeader {
    pub fn valid_magic(&self) -> bool {
        // magic number
        self.magic == MAGIC_NUMBER
    }

    pub fn get_off_dt_struct(&self) -> u32 {
        self.off_dt_struct
    }

    pub fn get_off_dt_strings(&self) -> u32 {
        self.off_dt_strings
    }

    // pub fn get_size_dt_struct(&self) -> u32 {
    //     self.size_dt_struct
    // }

    // pub fn get_size_dt_strings(&self) -> u32 {
    //     self.size_dt_strings
    // }

    // pub fn get_struct_range(&self) -> core::ops::Range<usize> {
    //     let start = self.off_dt_struct as usize;
    //     let end = start + self.size_dt_struct as usize;

    //     start..end
    // }

    // pub fn get_strings_range(&self) -> core::ops::Range<usize> {
    //     let start = self.off_dt_strings as usize;
    //     let end = start + self.size_dt_strings as usize;

    //     start..end
    // }

    pub fn load(dtb_addr: u32) -> FdtHeader {
        let header_ptr = unsafe { &*(dtb_addr as *const FdtHeader) };
        // cause device tree use big-endian format,
        // so we need to use swap_bytes to change to little-endian
        // println!("[fdt_header] Load magic: {:#x}", header_ptr.magic.swap_bytes());
        // println!("[fdt_header] Load totalsize: {:#x}", header_ptr.totalsize.swap_bytes());
        // println!("[fdt_header] Load off_dt_struct: {:#x}", header_ptr.off_dt_struct.swap_bytes());
        // println!("[fdt_header] Load off_dt_strings: {:#x}", header_ptr.off_dt_strings.swap_bytes());
        // println!("[fdt_header] Load version: {:#x}", header_ptr.version.swap_bytes());
        // println!("[fdt_header] Load last_comp_version: {:#x}", header_ptr.last_comp_version.swap_bytes());
        // println!("[fdt_header] Load boot_cpuid_phys: {:#x}", header_ptr.boot_cpuid_phys.swap_bytes());
        // println!("[fdt_header] Load size_dt_strings: {:#x}", header_ptr.size_dt_strings.swap_bytes());
        // println!("[fdt_header] Load size_dt_struct: {:#x}", header_ptr.size_dt_struct.swap_bytes());
        FdtHeader {
            magic: header_ptr.magic.swap_bytes(),
            totalsize: header_ptr.totalsize.swap_bytes(),
            off_dt_struct: header_ptr.off_dt_struct.swap_bytes(),
            off_dt_strings: header_ptr.off_dt_strings.swap_bytes(),
            off_mem_rsvmap: header_ptr.off_mem_rsvmap.swap_bytes(),
            version: header_ptr.version.swap_bytes(),
            last_comp_version: header_ptr.last_comp_version.swap_bytes(),
            boot_cpuid_phys: header_ptr.boot_cpuid_phys.swap_bytes(),
            size_dt_strings: header_ptr.size_dt_strings.swap_bytes(),
            size_dt_struct: header_ptr.size_dt_struct.swap_bytes(),
        }
    }
}
