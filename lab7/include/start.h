#pragma once

#define PAGE_SIZE 0x1000
#define STACK_SIZE 0x4000

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

/*
 * nGnRnE: Non-Gathering, Non-Reordering, No Early Write Acknowledgement
 * Normal memory, Inner Non-cacheable, Outer Non-cacheable
 */
#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_ENTRY 0b11

// Bits[10] The access flag, a page fault is generated if not set.
#define PD_ACCESS (1 << 10)

// Bits[7] 0 for read-write, 1 or read-only
#define PD_RDONLY (1 << 7)

// Bits[6] 0 for only kernel access, 1 for user/kernel access
#define PD_UKACCESS (1 << 6)

// Bits[4:2] The index to MAIR
#define PD_MAIR_DEVICE_nGnRnE (MAIR_IDX_DEVICE_nGnRnE << 2)
#define PD_MAIR_NORMAL_NOCACHE (MAIR_IDX_NORMAL_NOCACHE << 2)

#define BOOT_PGD_PAGE_FRAME 0x1000  // Boot PGD's page frame
#define BOOT_PUD_PAGE_FRAME 0x2000  // Boot PUD's page frame
#define BOOT_PMD_PAGE_FRAME 0x3000  // Boot PMD's page frame

#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR PD_TABLE
#define BOOT_PUD_DEVICE_ATTR (PD_ACCESS | PD_MAIR_DEVICE_nGnRnE | PD_BLOCK)
#define BOOT_PMD_NORMAL_ATTR (PD_ACCESS | PD_MAIR_NORMAL_NOCACHE | PD_BLOCK)
#define BOOT_PMD_DEVICE_ATTR (PD_ACCESS | PD_MAIR_DEVICE_nGnRnE | PD_BLOCK)