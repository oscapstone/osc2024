use core::{arch::asm, array, fmt, mem::size_of};

use aarch64_cpu::{asm::barrier, registers::*};
use alloc::{collections::LinkedList, vec};
use bsp::memory::PERIPHERAL_MMIO_BASE;
use library::{
    console::{console, ConsoleMode},
    println,
};
use tock_registers::{interfaces::ReadWriteable, register_bitfields, registers::InMemoryRegister};

use crate::memory::{phys_to_virt, virt_to_phys, AllocatedMemory, PAGE_SIZE};

use super::memory_mapping::MemoryExecutePermission;

register_bitfields! [
    u64,
    TABLE_DESCRIPTOR [
        /// Physical address of the next descriptor.
        NEXT_LEVEL_TABLE_ADDR OFFSET(12) NUMBITS(36) [], // [47:12]
        TYPE OFFSET(1) NUMBITS(1) [
            Block = 0,
            Table = 1
        ],
        VALID OFFSET(0) NUMBITS(1) [
            False = 0,
            True = 1
        ]
    ]
];

register_bitfields! [
    u64,
    BLOCK_DESCRIPTOR_2MB [
        /// Physical address of the next descriptor.
        BLOCK_ADDR OFFSET(21) NUMBITS(27) [], // [47:21]
        /// Access flag.
        AF OFFSET(10) NUMBITS(1) [
            False = 0,
            True = 1
        ],
        TYPE OFFSET(1) NUMBITS(1) [
            Block = 0,
            Table = 1
        ],
        VALID OFFSET(0) NUMBITS(1) [
            False = 0,
            True = 1
        ]
    ],
    BLOCK_DESCRIPTOR_4KB [
        /// Physical address of the next descriptor.
        BLOCK_ADDR OFFSET(12) NUMBITS(36) [], // [47:12]
        /// Access flag.
        AF OFFSET(10) NUMBITS(1) [
            False = 0,
            True = 1
        ],
        TYPE OFFSET(1) NUMBITS(1) [
            Block = 0,
            Table = 1
        ],
        VALID OFFSET(0) NUMBITS(1) [
            False = 0,
            True = 1
        ]
    ]
];

register_bitfields! [
    u64,
    PAGE_DESCRIPTOR [
        /// Unprivileged execute-never.
        UXN OFFSET(54) NUMBITS(1) [
            False = 0,
            True = 1
        ],
        /// Privileged execute-never.
        PXN OFFSET(53) NUMBITS(1) [
            False = 0,
            True = 1
        ],
        /// Physical address of the next table descriptor (lvl2) or the page descriptor (lvl3).
        OUTPUT_ADDR OFFSET(12) NUMBITS(36) [], // [47:12]
        /// Access flag.
        AF OFFSET(10) NUMBITS(1) [
            False = 0,
            True = 1
        ],
        /// Shareability field.
        SH OFFSET(8) NUMBITS(2) [
            OuterShareable = 0b10,
            InnerShareable = 0b11
        ],
        /// Access Permissions.
        AP OFFSET(6) NUMBITS(2) [
            RW_EL1 = 0b00,
            RW_EL1_EL0 = 0b01,
            RO_EL1 = 0b10,
            RO_EL1_EL0 = 0b11
        ],
        /// Memory attributes index into the MAIR_EL1 register.
        AttrIndx OFFSET(2) NUMBITS(3) [],
        TYPE OFFSET(1) NUMBITS(1) [
            Reserved_Invalid = 0,
            Page = 1
        ],
        VALID OFFSET(0) NUMBITS(1) [
            False = 0,
            True = 1
        ]
    ]
];

/// A table descriptor.
///
/// The output points to the next table.
#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub struct TableDescriptor {
    value: u64,
}

impl TableDescriptor {
    pub const fn new() -> Self {
        Self { value: 0 }
    }

    pub fn from_next_level_table_addr(addr: usize) -> Self {
        let val = InMemoryRegister::<u64, TABLE_DESCRIPTOR::Register>::new(0);
        val.write(
            TABLE_DESCRIPTOR::NEXT_LEVEL_TABLE_ADDR.val(addr as u64 >> Granule4KiB::SHIFT)
                + TABLE_DESCRIPTOR::TYPE::Table
                + TABLE_DESCRIPTOR::VALID::True,
        );
        Self { value: val.get() }
    }

    pub fn from_2mb_block_addr(addr: usize) -> Self {
        let val = InMemoryRegister::<u64, BLOCK_DESCRIPTOR_2MB::Register>::new(0);
        val.write(
            BLOCK_DESCRIPTOR_2MB::BLOCK_ADDR.val(addr as u64 >> Granule2MiB::SHIFT)
                + BLOCK_DESCRIPTOR_2MB::AF::True
                + BLOCK_DESCRIPTOR_2MB::TYPE::Block
                + BLOCK_DESCRIPTOR_2MB::VALID::True,
        );
        Self { value: val.get() }
    }

    #[inline(always)]
    pub fn is_valid(&self) -> bool {
        InMemoryRegister::<u64, TABLE_DESCRIPTOR::Register>::new(self.value)
            .read(TABLE_DESCRIPTOR::VALID)
            != 0
    }

    #[inline(always)]
    pub fn next_level_addr(&self) -> u64 {
        InMemoryRegister::<u64, TABLE_DESCRIPTOR::Register>::new(self.value)
            .read(TABLE_DESCRIPTOR::NEXT_LEVEL_TABLE_ADDR)
            << Granule4KiB::SHIFT
    }
}

impl fmt::Display for TableDescriptor {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let reg = InMemoryRegister::<u64, TABLE_DESCRIPTOR::Register>::new(self.value);
        writeln!(f, "TableDescriptor {{")?;
        writeln!(
            f,
            "  NEXT_LEVEL_TABLE_ADDR: {:#x}",
            reg.read(TABLE_DESCRIPTOR::NEXT_LEVEL_TABLE_ADDR) << Granule4KiB::SHIFT
        )?;
        writeln!(
            f,
            "  TYPE: {}",
            match reg.read_as_enum(TABLE_DESCRIPTOR::TYPE) {
                Some(TABLE_DESCRIPTOR::TYPE::Value::Table) => "Table",
                Some(TABLE_DESCRIPTOR::TYPE::Value::Block) => "Block",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  VALID: {}",
            match reg.read_as_enum(TABLE_DESCRIPTOR::VALID) {
                Some(TABLE_DESCRIPTOR::VALID::Value::True) => "True",
                Some(TABLE_DESCRIPTOR::VALID::Value::False) => "False",
                _ => "Unknown",
            }
        )?;
        write!(f, "}}")
    }
}

#[derive(Debug, Clone, Copy)]
pub enum MemoryAttribute {
    Device,
    Normal,
}

#[derive(Debug, Clone, Copy)]
pub enum MemoryAccessPermission {
    ReadWriteEL1,
    ReadWriteEL1EL0,
    ReadOnlyEL1,
    ReadOnlyEL1EL0,
}

/// A page descriptor.
///
/// The output points to physical memory.
#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub struct PageDescriptor {
    value: u64,
}

impl PageDescriptor {
    pub const fn new() -> Self {
        Self { value: 0 }
    }

    pub fn from_output_addr(addr: usize) -> Self {
        let val = InMemoryRegister::<u64, PAGE_DESCRIPTOR::Register>::new(0);
        val.write(
            PAGE_DESCRIPTOR::OUTPUT_ADDR.val(addr as u64 >> 12)
                + PAGE_DESCRIPTOR::AF::True
                + PAGE_DESCRIPTOR::TYPE::Page
                + PAGE_DESCRIPTOR::VALID::True,
        );
        Self { value: val.get() }
    }

    pub fn set_attribute(&mut self, attr: MemoryAttribute) {
        let val = InMemoryRegister::<u64, PAGE_DESCRIPTOR::Register>::new(self.value);
        match attr {
            MemoryAttribute::Device => val.modify(PAGE_DESCRIPTOR::AttrIndx.val(0)),
            MemoryAttribute::Normal => val.modify(PAGE_DESCRIPTOR::AttrIndx.val(1)),
        }
        self.value = val.get();
    }

    #[inline(always)]
    pub fn is_valid(&self) -> bool {
        InMemoryRegister::<u64, PAGE_DESCRIPTOR::Register>::new(self.value)
            .read(PAGE_DESCRIPTOR::VALID)
            != 0
    }

    #[inline(always)]
    pub fn set_access_permission(&mut self, access_permission: MemoryAccessPermission) {
        let val = InMemoryRegister::<u64, PAGE_DESCRIPTOR::Register>::new(self.value);
        match access_permission {
            MemoryAccessPermission::ReadWriteEL1 => val.modify(PAGE_DESCRIPTOR::AP::RW_EL1),
            MemoryAccessPermission::ReadWriteEL1EL0 => val.modify(PAGE_DESCRIPTOR::AP::RW_EL1_EL0),
            MemoryAccessPermission::ReadOnlyEL1 => val.modify(PAGE_DESCRIPTOR::AP::RO_EL1),
            MemoryAccessPermission::ReadOnlyEL1EL0 => val.modify(PAGE_DESCRIPTOR::AP::RO_EL1_EL0),
        }
        self.value = val.get();
    }

    #[inline(always)]
    pub fn set_execute_permission(&mut self, execute_permission: MemoryExecutePermission) {
        let val = InMemoryRegister::<u64, PAGE_DESCRIPTOR::Register>::new(self.value);
        match execute_permission {
            MemoryExecutePermission::AllowKernelAndUser => {
                val.modify(PAGE_DESCRIPTOR::UXN::False + PAGE_DESCRIPTOR::PXN::False)
            }
            MemoryExecutePermission::AllowUser => {
                val.modify(PAGE_DESCRIPTOR::UXN::False + PAGE_DESCRIPTOR::PXN::True)
            }
            MemoryExecutePermission::AllowKernel => {
                val.modify(PAGE_DESCRIPTOR::UXN::True + PAGE_DESCRIPTOR::PXN::False)
            }
            MemoryExecutePermission::Deny => {
                val.modify(PAGE_DESCRIPTOR::UXN::True + PAGE_DESCRIPTOR::PXN::True)
            }
        }
        self.value = val.get();
    }

    #[inline(always)]
    pub fn output_addr(&self) -> u64 {
        InMemoryRegister::<u64, PAGE_DESCRIPTOR::Register>::new(self.value)
            .read(PAGE_DESCRIPTOR::OUTPUT_ADDR)
            << Granule4KiB::SHIFT
    }
}

impl fmt::Display for PageDescriptor {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let reg = InMemoryRegister::<u64, PAGE_DESCRIPTOR::Register>::new(self.value);
        writeln!(f, "PageDescriptor {{")?;
        writeln!(
            f,
            "  UXN: {}",
            match reg.read_as_enum(PAGE_DESCRIPTOR::UXN) {
                Some(PAGE_DESCRIPTOR::UXN::Value::True) => "True",
                Some(PAGE_DESCRIPTOR::UXN::Value::False) => "False",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  PXN: {}",
            match reg.read_as_enum(PAGE_DESCRIPTOR::PXN) {
                Some(PAGE_DESCRIPTOR::PXN::Value::True) => "True",
                Some(PAGE_DESCRIPTOR::PXN::Value::False) => "False",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  OUTPUT_ADDR: {:#x}",
            reg.read(PAGE_DESCRIPTOR::OUTPUT_ADDR) << Granule4KiB::SHIFT
        )?;
        writeln!(
            f,
            "  AF: {}",
            match reg.read_as_enum(PAGE_DESCRIPTOR::AF) {
                Some(PAGE_DESCRIPTOR::AF::Value::True) => "True",
                Some(PAGE_DESCRIPTOR::AF::Value::False) => "False",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  SH: {}",
            match reg.read_as_enum(PAGE_DESCRIPTOR::SH) {
                Some(PAGE_DESCRIPTOR::SH::Value::OuterShareable) => "OuterShareable",
                Some(PAGE_DESCRIPTOR::SH::Value::InnerShareable) => "InnerShareable",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  AP: {}",
            match reg.read_as_enum(PAGE_DESCRIPTOR::AP) {
                Some(PAGE_DESCRIPTOR::AP::Value::RW_EL1) => "RW_EL1",
                Some(PAGE_DESCRIPTOR::AP::Value::RW_EL1_EL0) => "RW_EL1_EL0",
                Some(PAGE_DESCRIPTOR::AP::Value::RO_EL1) => "RO_EL1",
                Some(PAGE_DESCRIPTOR::AP::Value::RO_EL1_EL0) => "RO_EL1_EL0",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  AttrIndx: {}",
            match reg.read(PAGE_DESCRIPTOR::AttrIndx) {
                0 => "Device",
                1 => "Normal",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  TYPE: {}",
            match reg.read_as_enum(PAGE_DESCRIPTOR::TYPE) {
                Some(PAGE_DESCRIPTOR::TYPE::Value::Page) => "Page",
                Some(PAGE_DESCRIPTOR::TYPE::Value::Reserved_Invalid) => "Reserved_Invalid",
                _ => "Unknown",
            }
        )?;
        writeln!(
            f,
            "  VALID: {}",
            match reg.read_as_enum(PAGE_DESCRIPTOR::VALID) {
                Some(PAGE_DESCRIPTOR::VALID::Value::True) => "True",
                Some(PAGE_DESCRIPTOR::VALID::Value::False) => "False",
                _ => "Unknown",
            }
        )?;
        write!(f, "}}")
    }
}

trait StartAddr {
    fn phys_start_addr_u64(&self) -> u64;
    fn phys_start_addr_usize(&self) -> usize;
}

impl<T, const N: usize> StartAddr for [T; N] {
    fn phys_start_addr_u64(&self) -> u64 {
        self.as_ptr() as u64
    }

    fn phys_start_addr_usize(&self) -> usize {
        self.as_ptr() as usize
    }
}

pub struct TranslationGranule<const GRANULE_SIZE: usize>;

impl<const GRANULE_SIZE: usize> TranslationGranule<GRANULE_SIZE> {
    /// The granule's size.
    pub const SIZE: usize = Self::size_checked();

    /// The granule's shift, aka log2(size).
    pub const SHIFT: usize = Self::SIZE.trailing_zeros() as usize;

    const fn size_checked() -> usize {
        assert!(GRANULE_SIZE.is_power_of_two());
        GRANULE_SIZE
    }
}

pub type Granule1GiB = TranslationGranule<{ 1 * 1024 * 1024 * 1024 }>;
pub type Granule2MiB = TranslationGranule<{ 2 * 1024 * 1024 }>;
pub type Granule4KiB = TranslationGranule<{ 4 * 1024 }>;

pub type PageGlobalDirectory = [TableDescriptor; NUM_ENTRIES];

const NUM_ENTRIES: usize = 4096 / size_of::<PageDescriptor>();

/// A fixed-size translation table.
/// The table use 4 level translation and have fewer tables because linear address space < 0x7FFF_FFFF.
/// PGD: 1 entry PUD: 2 entries PMD: 2 * 512 entries PT: 2 * 512 * 512 entries
#[repr(C)]
#[repr(align(4096))]
pub struct FixedSizeTranslationTable {
    // 1GB
    pud: [TableDescriptor; NUM_ENTRIES],
    // 2MB (we need at least 2 of then to cover 1 GB RAM and 1 GB MMIO)
    pmd: [[TableDescriptor; NUM_ENTRIES]; 2],
    // 4KB
    pt: [[[PageDescriptor; NUM_ENTRIES]; NUM_ENTRIES]; 2],
    // should be align to 4 KB, so put it at the end (512 GB)
    pgd: TableDescriptor,
}

impl FixedSizeTranslationTable {
    pub const fn new() -> Self {
        Self {
            pgd: TableDescriptor::new(),
            pud: [TableDescriptor::new(); NUM_ENTRIES],
            pmd: [[TableDescriptor::new(); NUM_ENTRIES]; 2],
            pt: [[[PageDescriptor::new(); NUM_ENTRIES]; NUM_ENTRIES]; 2],
        }
    }

    pub fn populate_table_entries(&mut self) {
        self.pgd = TableDescriptor::from_next_level_table_addr(self.pud.phys_start_addr_usize());
        self.pud = array::from_fn(|i| {
            // we only need 2 pud entries
            // so just ignore the rest invalid entries
            TableDescriptor::from_next_level_table_addr(
                self.pmd.phys_start_addr_usize() + i * Granule4KiB::SIZE,
            )
        });
        self.pmd = array::from_fn(|i| {
            array::from_fn(|j| {
                TableDescriptor::from_next_level_table_addr(
                    self.pt.phys_start_addr_usize() + i * Granule2MiB::SIZE + j * Granule4KiB::SIZE,
                )
            })
        });
        // stack size is not enough to use array::from_fn
        for i in 0..2 {
            for (j, pmd) in self.pmd[i].iter().enumerate() {
                for (k, pte) in self.pt[i][j].iter_mut().enumerate() {
                    let virt_addr =
                        i << Granule1GiB::SHIFT | j << Granule2MiB::SHIFT | k << Granule4KiB::SHIFT;
                    let mut t = PageDescriptor::from_output_addr(virt_addr);
                    if virt_addr >= PERIPHERAL_MMIO_BASE {
                        t.set_attribute(MemoryAttribute::Device);
                    } else {
                        t.set_attribute(MemoryAttribute::Normal);
                    }
                    *pte = t;
                }
            }
        }
    }

    pub fn phys_base_address(&self) -> u64 {
        &self.pgd as *const _ as u64
    }
}

pub type KernelTranslationTable = FixedSizeTranslationTable;

#[derive(Debug)]
pub struct PageTable {
    pgd: *mut PageGlobalDirectory,
    allocated_pages: LinkedList<AllocatedMemory>,
}

impl PageTable {
    const MAX_LEVEL: usize = 4;

    pub fn new() -> Self {
        let pgd_mem = AllocatedMemory::new(
            vec![0_u8; size_of::<TableDescriptor>() * NUM_ENTRIES].into_boxed_slice(),
        );
        let pgd = pgd_mem.as_ptr() as *mut PageGlobalDirectory;
        let mut allocated_pages = LinkedList::new();
        allocated_pages.push_back(pgd_mem);
        Self {
            pgd,
            allocated_pages,
        }
    }

    #[inline(always)]
    pub fn phys_base_address(&self) -> u64 {
        virt_to_phys(self.pgd as usize) as u64
    }

    fn map_page(&mut self, virt_addr: usize, pte: PageDescriptor) {
        let mut table = unsafe { &mut *self.pgd };
        for level in 0..Self::MAX_LEVEL {
            let shift = 9 * (Self::MAX_LEVEL - 1 - level) + 12;
            let index = (virt_addr >> shift) & 0b1_1111_1111;
            let entry = table[index];
            if level == Self::MAX_LEVEL - 1 {
                // pt
                let pt = unsafe {
                    &mut *(table as *mut [TableDescriptor; NUM_ENTRIES]
                        as *mut [PageDescriptor; NUM_ENTRIES])
                };
                pt[index] = pte;
            } else {
                // pgd, pud, pmd
                if entry.is_valid() {
                    table = unsafe {
                        &mut *(phys_to_virt(entry.next_level_addr() as usize)
                            as *mut [TableDescriptor; NUM_ENTRIES])
                    };
                } else {
                    // create next level table first
                    let next_level_table_mem = AllocatedMemory::new(
                        vec![0_u8; size_of::<TableDescriptor>() * NUM_ENTRIES].into_boxed_slice(),
                    );
                    let next_level_table = next_level_table_mem.as_ptr() as usize;
                    self.allocated_pages.push_back(next_level_table_mem);
                    console().change_mode(ConsoleMode::Sync);
                    println!(
                        "Allocate a level {} table at {:#x}",
                        level + 1,
                        next_level_table
                    );
                    console().change_mode(ConsoleMode::Async);
                    table[index] =
                        TableDescriptor::from_next_level_table_addr(virt_to_phys(next_level_table));
                    table =
                        unsafe { &mut *(next_level_table as *mut [TableDescriptor; NUM_ENTRIES]) };
                }
            }
        }
    }

    #[inline(always)]
    fn flush_tlb() {
        barrier::dsb(barrier::ISH);
        unsafe { asm!("tlbi vmalle1is") };
        barrier::dsb(barrier::ISH);
        barrier::isb(barrier::SY);
    }

    pub fn map_pages(
        &mut self,
        virt_addr: usize,
        phys_addr: usize,
        size: usize,
        memory_attribute: MemoryAttribute,
        access_permission: MemoryAccessPermission,
        execute_permission: MemoryExecutePermission,
    ) -> Result<(), &'static str> {
        if virt_addr % PAGE_SIZE != 0 || phys_addr % PAGE_SIZE != 0 || size % PAGE_SIZE != 0 {
            return Err("Address or size is not page aligned");
        }

        for offset in (0..size).step_by(PAGE_SIZE) {
            let mut pte = PageDescriptor::from_output_addr(phys_addr + offset);
            pte.set_attribute(memory_attribute);
            pte.set_access_permission(access_permission);
            pte.set_execute_permission(execute_permission);
            self.map_page(virt_addr + offset, pte);
        }

        Self::flush_tlb();
        Ok(())
    }

    pub fn unmap_pages(&mut self, virt_addr: usize, size: usize) -> Result<(), &'static str> {
        if virt_addr % PAGE_SIZE != 0 || size % PAGE_SIZE != 0 {
            return Err("Address or size is not page aligned");
        }

        for offset in (0..size).step_by(PAGE_SIZE) {
            self.map_page(virt_addr + offset, PageDescriptor::new());
        }

        Self::flush_tlb();
        Ok(())
    }

    /// Translate physical address to virtual address by the provided page table physical address.
    /// # Safety
    /// - The provided page table must be valid.
    /// - Ensure the page table is not modified during the translation.
    pub unsafe fn virt_to_phys_by_table(
        table_phys_addr: usize,
        virt_addr: usize,
    ) -> Result<usize, &'static str> {
        let mut table = unsafe { &*(phys_to_virt(table_phys_addr) as *const PageGlobalDirectory) };
        for level in 0..Self::MAX_LEVEL - 1 {
            let shift = 9 * (Self::MAX_LEVEL - 1 - level) + 12;
            let index = (virt_addr >> shift) & 0b1_1111_1111;
            // pgd, pud, pmd
            let entry = table[index];
            if entry.is_valid() {
                table = unsafe {
                    &mut *(phys_to_virt(entry.next_level_addr() as usize)
                        as *mut [TableDescriptor; NUM_ENTRIES])
                };
            } else {
                return Err("Page not mapped");
            }
        }

        // pt
        let pt = unsafe {
            &*(table as *const [TableDescriptor; NUM_ENTRIES]
                as *const [PageDescriptor; NUM_ENTRIES])
        };
        let index = (virt_addr >> Granule4KiB::SHIFT) & 0b1_1111_1111;
        let entry = pt[index];
        if entry.is_valid() {
            return Ok(entry.output_addr() as usize | (virt_addr & 0xfff));
        } else {
            return Err("Page not mapped");
        }
    }
}
