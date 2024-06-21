#ifndef _MMU_H_
#define _MMU_H_

#include "stddef.h"

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE 0b11L
#define PD_BLOCK 0b01L
#define PD_UNX (1L << 54)
#define PD_ACCESS (1L << 10)
#define PD_UK_ACCESS (1L << 6)

#define PERIPHERAL_START 0x3c000000L
#define PERIPHERAL_END 0x3f000000L
#define USER_KERNEL_BASE 0x00000000L
#define USER_STACK_BASE 0xfffffffff000L

#define MMU_PGD_BASE 0x1000L
#define MMU_PGD_ADDR (MMU_PGD_BASE + 0x0000L)
#define MMU_PUD_ADDR (MMU_PGD_BASE + 0x1000L)
#define MMU_PTE_ADDR (MMU_PGD_BASE + 0x2000L)

#define BOOT_PGD_ATTR (PD_TABLE)
#define BOOT_PUD_ATTR (PD_TABLE + PD_ACCESS + (MAIR_IDX_NORMAL_NOCACHE << 2))
#define BOOT_PTE_ATTR_nGnRnE (PD_BLOCK + PD_ACCESS + PD_UK_ACCESS + (MAIR_DEVICE_nGnRnE << 2))
#define BOOT_PTE_ATTR_NOCACHE (PD_BLOCK + PD_ACCESS + (MAIR_NORMAL_NOCACHE << 2))

#ifndef __ASSEMBLER__

void *set_2M_kernel_mmu(void *x0);
void map_one_page(size_t *pgd_p, size_t va, size_t pa);
void mappages(size_t *pgd_p, size_t va, size_t size, size_t pa);
void free_page_tables(size_t *page_table, int level);

#endif //__ASSEMBLER__

#endif /* _MMU_H_ */
