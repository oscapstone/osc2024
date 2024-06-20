#ifndef _MMU_H_
#define _MMU_H_

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_UNX                  (1L << 54)                                  // non-executable page frame for EL0 if set
#define PD_KNX                  (1L << 53)                                  // non-executable page frame for EL1 if set
#define PD_ACCESS (1 << 10)
#define BOOT_PGD_ATTR PD_TABLE
#define USER_ACCESS (1 << 6)
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_PMD_ATTR_nGnRnE (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_PMD_ATTR_NORMAL_NOCACHE (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK)
#define USER_ATTR_nGnRnE (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_TABLE | USER_ACCESS)
#define USER_ATTR_NORMAL_NOCACHE (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_TABLE | USER_ACCESS)

#define KERNEL_PGD_BASE 0x1000
#define KERNEL_PUD_BASE 0x2000
#define KERNEL_PMD_1_BASE 0x3000
#define KERNEL_PMD_2_BASE 0x4000

#define PERIPHERAL_START        0x3c000000
#define PERIPHERAL_END          0x3f000000
#define USER_PROC_BASE        0x00000000
#define USER_STACK_BASE         0xfffffffff000

#define TABLE_ADDRESS_MASK 0x0000fffffffff000

#ifndef __ASSEMBLER__

int set_2M_kernel_mmu();
unsigned long* get_new_page();
void Identity_Paging_el0();
int map_pages(unsigned long pgd, unsigned long va, unsigned long pa, unsigned int size,unsigned long flag);
unsigned long va_to_pa(unsigned long pgd,unsigned long va);
int reset_pgd(unsigned long pgd,int level);

#endif //for __ASSEMBLER__
#endif //for _MMU_H_