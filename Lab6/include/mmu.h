#ifndef __MMU_H__
#define __MMU_H__

/* Translation Control Register (TCR) */
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16)) // set T0SZ and T1SZ
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30)) // set TG0 and TG1 to 4KB
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

/* Memory Attribute Indirection Register (MAIR) */
#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

/* PGD and PUD */
#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_PAGE 0b11

#define USER_ACCESS (1 << 6) // 0: only kernel access, 1: kernel/user access
#define PD_RDONLY (1 << 7) // 0: read-write, 1: read-only
#define PD_ACCESS (1 << 10) // a page fault will be generated if not set
#define PD_KNX (1L << 53) // non-executable in EL1
#define PD_UNX (1L << 54) // non-executable in EL0

#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_NORMAL_ATTR (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK)
#define BOOT_DEVICE_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#endif