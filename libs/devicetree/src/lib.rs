#![no_std]

use core::{
    mem::{align_of, size_of},
    slice, str,
};

pub const DEVICETREE_MAGIC: u32 = 0xd00dfeed;

#[repr(packed)]
#[derive(Debug)]
pub struct FlattenedDevicetreeHeader {
    magic: u32,
    total_size: u32,
    structure_block_offset: u32,
    strings_block_offset: u32,
    memory_reserve_offset: u32,
    version: u32,
    last_compatible_version: u32,
    boot_cpuid_physical: u32,
    strings_block_size: u32,
    structure_block_size: u32,
}

impl FlattenedDevicetreeHeader {
    pub const fn magic(&self) -> u32 {
        u32::from_be(self.magic)
    }

    pub const fn total_size(&self) -> u32 {
        u32::from_be(self.total_size)
    }

    pub const fn structure_block_offset(&self) -> u32 {
        u32::from_be(self.structure_block_offset)
    }

    pub const fn strings_offset(&self) -> u32 {
        u32::from_be(self.strings_block_offset)
    }

    pub const fn memory_reserve_offset(&self) -> u32 {
        u32::from_be(self.memory_reserve_offset)
    }

    pub const fn version(&self) -> u32 {
        u32::from_be(self.version)
    }

    pub const fn last_compatible_version(&self) -> u32 {
        u32::from_be(self.last_compatible_version)
    }

    pub const fn boot_cpuid_physical(&self) -> u32 {
        u32::from_be(self.boot_cpuid_physical)
    }

    pub const fn strings_block_size(&self) -> u32 {
        u32::from_be(self.strings_block_size)
    }

    pub const fn structure_block_size(&self) -> u32 {
        u32::from_be(self.structure_block_size)
    }
}

#[repr(u32)]
#[derive(Debug)]
enum FlattenedDevicetreeStructureTokenType {
    FdtBeginNode = 0x00000001,
    FdtEndNode = 0x00000002,
    FdtProp = 0x00000003,
    FdtNop = 0x00000004,
    FdtEnd = 0x00000009,
}

impl From<u32> for FlattenedDevicetreeStructureTokenType {
    fn from(value: u32) -> Self {
        unsafe { core::mem::transmute(value) }
    }
}

#[repr(packed)]
#[derive(Debug)]
struct FlattenedDevicetreeProperty {
    len: u32,
    name_offset: u32,
}

impl FlattenedDevicetreeProperty {
    pub const fn len(&self) -> u32 {
        u32::from_be(self.len)
    }

    pub const fn name_offset(&self) -> u32 {
        u32::from_be(self.name_offset)
    }
}

pub struct FlattenedDevicetree {
    start_addr: usize,
    current: usize,
}

impl FlattenedDevicetree {
    pub const unsafe fn from_memory(mmio_start_addr: usize) -> Self {
        Self {
            start_addr: mmio_start_addr,
            current: mmio_start_addr,
        }
    }

    pub fn traverse(
        &mut self,
        device_init_callback: &impl Fn(&str, &str, &[u8]) -> Result<(), &'static str>,
    ) -> Result<(), &'static str> {
        let header = unsafe { &*(self.start_addr as *const FlattenedDevicetreeHeader) };
        if header.magic() != DEVICETREE_MAGIC {
            return Err("Not a valid Flattened Devicetree");
        }
        self.current = self.start_addr + header.structure_block_offset() as usize;
        self.traverse_node(header, None, device_init_callback)?;
        Ok(())
    }

    fn traverse_node(
        &mut self,
        header: &FlattenedDevicetreeHeader,
        device_name: Option<&str>,
        device_init_callback: &impl Fn(&str, &str, &[u8]) -> Result<(), &'static str>,
    ) -> Result<(), &'static str> {
        unsafe {
            let structure_block_end = self.start_addr
                + header.structure_block_offset() as usize
                + header.structure_block_size() as usize;
            while self.current + size_of::<u32>() < structure_block_end {
                let token_type = FlattenedDevicetreeStructureTokenType::from(u32::from_be(
                    *(self.current as *const u32),
                ));
                self.current += size_of::<u32>();
                match token_type {
                    FlattenedDevicetreeStructureTokenType::FdtBeginNode => {
                        let device_name = self.parse_device_name();
                        self.traverse_node(header, Some(device_name), device_init_callback)?;
                    }
                    FlattenedDevicetreeStructureTokenType::FdtProp => {
                        let property = &*(self.current as *const FlattenedDevicetreeProperty);
                        self.current += size_of::<FlattenedDevicetreeProperty>();
                        device_init_callback(
                            device_name.unwrap(),
                            self.parse_property_name(
                                (self.start_addr
                                    + header.strings_offset() as usize
                                    + property.name_offset() as usize)
                                    as *const u8,
                            ),
                            self.parse_property_value(property.len()),
                        )?;
                    }
                    FlattenedDevicetreeStructureTokenType::FdtEndNode => {
                        return Ok(());
                    }
                    FlattenedDevicetreeStructureTokenType::FdtNop => (),
                    FlattenedDevicetreeStructureTokenType::FdtEnd => break,
                    _ => return Err("Invalid Flattened Devicetree"),
                };
            }
            Ok(())
        }
    }

    fn parse_device_name(&mut self) -> &'static str {
        let device_name_start_addr = self.current as *const u8;
        let mut len = 0;
        unsafe {
            while *(self.current as *const u8) != 0 {
                len += 1;
                self.current += size_of::<u8>();
            }
            // do aligning
            self.current += match (self.current as *const u8).align_offset(align_of::<u32>()) {
                0 => 4,
                n => n,
            };
            str::from_utf8(slice::from_raw_parts(device_name_start_addr, len)).unwrap()
        }
    }

    fn parse_property_name(&self, property_name_start_addr: *const u8) -> &'static str {
        let mut current = property_name_start_addr;
        let mut len = 0;
        unsafe {
            while *current != 0 {
                len += 1;
                current = current.add(1);
            }
            str::from_utf8(slice::from_raw_parts(property_name_start_addr, len)).unwrap()
        }
    }

    fn parse_property_value(&mut self, len: u32) -> &[u8] {
        unsafe {
            let property_value = slice::from_raw_parts(self.current as *const u8, len as usize);
            self.current += len as usize;
            // do aligning
            self.current += (self.current as *const u8).align_offset(align_of::<u32>());
            property_value
        }
    }
}
