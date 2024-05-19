#ifndef _VM_MACRO_H
#define _VM_MACRO_H

#define KERNEL_VIRT_BASE 0xFFFF000000000000

#define TTBR0_EL1_REGION_BIT 48  // TTBR1_EL0 region size: 2^48 bytes
#define TTBR1_EL1_REGION_BIT 48  // TTBR1_EL1 region size: 2^48 bytes
#define TTBR0_EL1_GRANULE 0b00   // TTBR1_EL0 Granule size: 4KB
#define TTBR1_EL1_GRANULE 0b10   // TTBR1_EL1 Granule size: 4KB
#define TCR_EL1_T0SZ \
  ((64 - TTBR0_EL1_REGION_BIT) << 0)  // The size offsets of the memory regions
#define TCR_EL1_T1SZ \
  ((64 - TTBR1_EL1_REGION_BIT) << 16)  // addressed by TTBR0_EL1 and TTBR_EL1.
#define TCR_EL1_TG0 (TTBR0_EL1_GRANULE << 14)  // bits [15:14]: Granule size for
#define TCR_EL1_TG1 (TTBR1_EL1_GRANULE << 30)
#define TCR_EL1_VALUE (TCR_EL1_T0SZ | TCR_EL1_T1SZ | TCR_EL1_TG0 | TCR_EL1_TG1)

// Memory Attribute Indirection Register
#define MAIR_DEVICE_nGnRnE \
  0b00000000  // Attr<n>[7:4]==0b0000: Device memory
              // Attr<n>[3:0]==0b0000: Device-nGnRnE memory
#define MAIR_NORMAL_NOCACHE \
  0b01000100  // Attr<n>[7:4]==0b0100: Normal Memory, Outer Non-Cacheable
              // Attr<n>[3:0]==0b0100: Normal memory, Inner Non-Cacheable
#define MAIR_IDX_DEVICE_nGnRnE 0   // Device memory
#define MAIR_IDX_NORMAL_NOCACHE 1  // Normal memory without cache
#define MAIR_ATTR_0_VALUE (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8))
#define MAIR_ATTR_1_VALUE (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8))
#define MAIR_EL1_VALUE (MAIR_ATTR_1_VALUE | MAIR_ATTR_0_VALUE)

#define PAGE_SIZE 4096
#define PAGE_MASK ~0xFFF
#define PD_MASK 0x1FFULL

#define PGD_SHIFT 39
#define PUD_SHIFT 30
#define PMD_SHIFT 21
#define PTE_SHIFT 12

// Page Descriptor
#define PD_TABLE \
  0b11  // A page frame whose entries point to the next level page tables,
        // blocks, or pages.
#define PD_BLOCK \
  0b01  // A chunk of virtual memory pointed by one entry of PUD or PMD.
#define PD_PAGE 0b11  // A chunk of virtual memory pointed by one entry of PTE.
#define PD_ACCESS_FLAG \
  (1 << 10)  // The access flag, a page fault is generated if not set.
#define PD_USER_RW_FLAG \
  (0b01 << 6)  // user can read/write the region, then the kernel is
               // automatically not executable in that region no matter what the
               // value of Bits[53] is.

#define BOOT_PGD_ATTR PD_TABLE  // Lower attributes is ignored
#define BOOT_PUD0_ATTR PD_TABLE
#define BOOT_PUD1_ATTR \
  (PD_ACCESS_FLAG | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_PMD_ATTR PD_TABLE
#define PTE_DEVICE_ATTR \
  (PD_ACCESS_FLAG | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_PAGE)
#define PTE_NORAL_ATTR \
  (PD_ACCESS_FLAG | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE)

#define SCTLR_VALUE_MMU_DISABLED 0
#define SCTLR_VALUE_MMU_ENABLE 1

#

#endif