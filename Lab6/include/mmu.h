#ifndef _MMU_H_
#define _MMU_H_

#include "stddef.h"
                                                                            // tcr_el1: The control register for stage 1 of the EL1&0 translation regime.
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))      // T0SZ[5:0]   The size offset for ttbr0_el1 is 2**(64-T0SZ): 0x0000_0000_0000_0000 <- 0x0000_FFFF_FFFF_FFFF
#define TCR_CONFIG_4KB          ((0b00 << 14) | (0b10 << 30))               // T1SZ[21:16] The size offset for ttbr1_el1 is 2**(64-T1SZ): 0xFFFF_0000_0000_0000 -> 0xFFFF_FFFF_FFFF_FFFF
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)  // TG0[15:14]  Granule size for the TTBR0_EL1: 0b00 = 4KB // TG1[31:30]  Granule size for the TTBR1_EL1: 0b10 = 4KB

#define MAIR_DEVICE_nGnRnE          0b00000000
#define MAIR_NORMAL_NOCACHE         0b01000100
#define MAIR_IDX_DEVICE_nGnRnE      0
#define MAIR_IDX_NORMAL_NOCACHE     1

#define PD_TABLE                0b11L
#define PD_BLOCK                0b01L
#define PD_UNX                  (1L<<54)    //The unprivileged execute-never bit, non-executable page frame for EL0 if set.
#define PD_KNX                  (1L << 53)  //The privileged execute-never bit, non-executable page frame for EL1 if set.
#define PD_ACCESS               (1L << 10)  //The access flag, a page fault is generated if not set.
#define PD_RDONLY               (1L << 7)   //0 for read-write, 1 for read-only.
#define PD_UK_ACCESS            (1L << 6)   //0 for only kernel access, 1 for user/kernel access.

#define PERIPHERAL_START        0x3c000000L
#define PERIPHERAL_END          0x3f000000L
#define USER_KERNEL_BASE        0x00000000L
#define USER_STACK_BASE         0xfffffffff000L
#define USER_SIGNAL_WRAPPER_VA  0xffffffff9000L

#define MMU_PGD_BASE            0x1000L
#define MMU_PGD_ADDR            (MMU_PGD_BASE + 0x0000L)
#define MMU_PUD_ADDR            (MMU_PGD_BASE + 0x1000L)
#define MMU_PMD_ADDR            (MMU_PGD_BASE + 0x2000L)

// Used for EL1
#define BOOT_PGD_ATTR           (PD_TABLE)
#define BOOT_PUD_ATTR           (PD_TABLE | PD_ACCESS)
#define BOOT_PMD_ATTR_nGnRnE    (PD_BLOCK | PD_ACCESS | (MAIR_DEVICE_nGnRnE << 2) | PD_UNX | PD_KNX | PD_UK_ACCESS)
#define BOOT_PMD_ATTR_NOCACHE   (PD_BLOCK | PD_ACCESS | (MAIR_NORMAL_NOCACHE << 2))
#ifndef __ASSEMBLER__

#include "sched.h"
#include "exception.h"

typedef struct vm_area_struct
{
    list_head_t listhead;
    unsigned long virt_addr;
    unsigned long phys_addr;
    unsigned long area_size;
    unsigned long rwx;   // 1, 2, 4
    int is_alloced;

} vm_area_struct_t;

void *set_2M_kernel_mmu(void* x0);
void map_one_page(size_t *pgd_p, size_t va, size_t pa, size_t flag);

void mmu_add_vma(struct thread *t, size_t va, size_t size, size_t pa, size_t rwx, int is_alloced);
void mmu_del_vma(struct thread *t);
void mmu_map_pages(size_t *pgd_p, size_t va, size_t size, size_t pa, size_t flag);
void mmu_free_page_tables(size_t *page_table, int level);

void mmu_memfail_abort_handle(esr_el1_t* esr_el1);

#endif //__ASSEMBLER__

#endif /* _MMU_H_ */