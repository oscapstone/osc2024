#![allow(dead_code)]

const FDT_MAGIC: u32 = 0xD00D_FEED;

#[repr(packed)]
#[derive(Debug)]
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
    /// The magic number of the FDT header. This should be `0xd00dfeed`.
    pub fn magic(&self) -> u32 {
        u32::from_be(self.magic)
    }

    /// Whether the magic number is valid.
    pub fn is_valid(&self) -> bool {
        self.magic() == FDT_MAGIC
    }

    /// The total size in bytes of the FDT.
    pub fn totalsize(&self) -> u32 {
        u32::from_be(self.totalsize)
    }

    /// The offset in bytes of the structure block from the beginning of the header.
    pub fn off_dt_struct(&self) -> u32 {
        u32::from_be(self.off_dt_struct)
    }

    /// The offset in bytes of the strings block from the beginning of the header.
    pub fn off_dt_strings(&self) -> u32 {
        u32::from_be(self.off_dt_strings)
    }

    /// The offset in bytes of the memory reservation block from the beginning of the header.
    pub fn off_mem_rsvmap(&self) -> u32 {
        u32::from_be(self.off_mem_rsvmap)
    }

    /// The version of the FDT. This should be `17`.
    pub fn version(&self) -> u32 {
        u32::from_be(self.version)
    }

    /// The last compatible version of the FDT. This should be `16`.
    pub fn last_comp_version(&self) -> u32 {
        u32::from_be(self.last_comp_version)
    }

    /// The physical ID of the system's boot CPU.
    pub fn boot_cpuid_phys(&self) -> u32 {
        u32::from_be(self.boot_cpuid_phys)
    }

    /// The length in bytes of the strings block.
    pub fn size_dt_strings(&self) -> u32 {
        u32::from_be(self.size_dt_strings)
    }

    /// The length in bytes of the structure block.
    pub fn size_dt_struct(&self) -> u32 {
        u32::from_be(self.size_dt_struct)
    }
}

#[repr(packed)]
#[derive(Debug)]
struct FdtReserveEntry {
    address: u64,
    size: u64,
}

#[repr(u32)]
#[derive(Debug)]
pub enum StructureBlockToken {
    FdtBeginNode = 0x0000_0001,
    FdtEndNode = 0x0000_0002,
    FdtProp = 0x0000_0003,
    FdtNop = 0x0000_0004,
    FdtEnd = 0x0000_0009,
}

impl TryFrom<u32> for StructureBlockToken {
    type Error = u32;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            x if x == StructureBlockToken::FdtBeginNode as u32 => {
                Ok(StructureBlockToken::FdtBeginNode)
            }
            x if x == StructureBlockToken::FdtEndNode as u32 => Ok(StructureBlockToken::FdtEndNode),
            x if x == StructureBlockToken::FdtProp as u32 => Ok(StructureBlockToken::FdtProp),
            x if x == StructureBlockToken::FdtNop as u32 => Ok(StructureBlockToken::FdtNop),
            x if x == StructureBlockToken::FdtEnd as u32 => Ok(StructureBlockToken::FdtEnd),
            _ => Err(value),
        }
    }
}

#[repr(packed)]
#[derive(Debug)]
pub struct FdtProperty {
    len: u32,
    nameoff: u32,
}

impl FdtProperty {
    pub fn len(&self) -> u32 {
        u32::from_be(self.len)
    }

    pub fn nameoff(&self) -> u32 {
        u32::from_be(self.nameoff)
    }
}
