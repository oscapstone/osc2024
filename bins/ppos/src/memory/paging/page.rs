use core::{array, mem::size_of};

use aarch64_cpu::registers::*;
use tock_registers::{register_bitfields, registers::InMemoryRegister};

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
#[derive(Copy, Clone)]
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
            TABLE_DESCRIPTOR::NEXT_LEVEL_TABLE_ADDR.val(addr as u64 >> 12)
                + TABLE_DESCRIPTOR::TYPE::Table
                + TABLE_DESCRIPTOR::VALID::True,
        );
        Self { value: val.get() }
    }

    pub fn from_2mb_block_addr(addr: usize) -> Self {
        let val = InMemoryRegister::<u64, BLOCK_DESCRIPTOR_2MB::Register>::new(0);
        val.write(
            BLOCK_DESCRIPTOR_2MB::BLOCK_ADDR.val(addr as u64 >> 21)
                + BLOCK_DESCRIPTOR_2MB::AF::True
                + BLOCK_DESCRIPTOR_2MB::TYPE::Block
                + BLOCK_DESCRIPTOR_2MB::VALID::True,
        );
        Self { value: val.get() }
    }
}

/// A page descriptor.
///
/// The output points to physical memory.
#[derive(Copy, Clone)]
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

const NUM_TABLES: usize = 4096 / size_of::<PageDescriptor>();

/// A fixed-size translation table.
/// The table use 4 level translation and have fewer tables because linear address space < 0x7FFF_FFFF.
/// PGD: 1 entry PUD: 2 entries PMD: 2 * 512 entries PT: 2 * 512 * 512 entries
#[repr(C)]
#[repr(align(4096))]
pub struct FixedSizeTranslationTable {
    // 1GB
    pud: [TableDescriptor; NUM_TABLES],
    // 2MB (we need at least 2 of then to cover 1 GB RAM and 1 GB MMIO)
    pmd: [[TableDescriptor; NUM_TABLES]; 2],
    // 4KB
    pt: [[[PageDescriptor; NUM_TABLES]; NUM_TABLES]; 2],
    // should be align to 4 KB, so put it at the end (512 GB)
    pgd: TableDescriptor,
}

impl FixedSizeTranslationTable {
    pub const fn new() -> Self {
        Self {
            pgd: TableDescriptor::new(),
            pud: [TableDescriptor::new(); NUM_TABLES],
            pmd: [[TableDescriptor::new(); NUM_TABLES]; 2],
            pt: [[[PageDescriptor::new(); NUM_TABLES]; NUM_TABLES]; 2],
        }
    }

    pub fn populate_table_entries(&mut self) {
        self.pgd = TableDescriptor::from_next_level_table_addr(self.pud.phys_start_addr_usize());
        self.pud = array::from_fn(|i| {
            // we only need 2 pud entries
            // so just ignore the rest invalid entries
            TableDescriptor::from_next_level_table_addr(
                self.pmd.phys_start_addr_usize() + i * 0x1000,
            )
        });
        self.pmd = array::from_fn(|i| {
            array::from_fn(|j| {
                TableDescriptor::from_next_level_table_addr(
                    self.pt.phys_start_addr_usize() + i * NUM_TABLES * 0x1000 + j * 0x1000,
                )
            })
        });
        // stack size is not enough to use array::from_fn
        for i in 0..2 {
            for (j, pmd) in self.pmd[i].iter().enumerate() {
                for (k, pt) in self.pt[i][j].iter_mut().enumerate() {
                    *pt = PageDescriptor::from_output_addr(
                        i << Granule1GiB::SHIFT | j << Granule2MiB::SHIFT | k << Granule4KiB::SHIFT,
                    );
                }
            }
        }
    }

    pub fn phys_base_address(&self) -> u64 {
        &self.pgd as *const _ as u64
    }
}

pub type KernelTranslationTable = FixedSizeTranslationTable;
