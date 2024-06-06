#ifndef _MMU_H_
#define _MMU_H_

#include "stddef.h"
// tcr_el1: The control register for stage 1 of the EL1&0 translation regime.
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16)) // T0SZ 和 T1SZ 設為 16
#define TCR_CONFIG_4KB ((0b00 << 14) | (0b10 << 30))                   // TG0 和 TG1 設為 4KB
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000  // 設備記憶體，無全局無讀取無寫入權限
#define MAIR_NORMAL_NOCACHE 0b01000100 // 普通記憶體，不緩存
#define MAIR_IDX_DEVICE_nGnRnE 0       // 設備記憶體索引
#define MAIR_IDX_NORMAL_NOCACHE 1      // 普通記憶體索引

#define PD_TABLE 0b11L         // Table Entry Armv8_a_address_translation p.14
#define PD_BLOCK 0b01L         // Block Entry
#define PD_UNX (1L << 54)      // non-executable page frame for EL0 if set
#define PD_KNX (1L << 53)      // non-executable page frame for EL1 if set
#define PD_ACCESS (1L << 10)   // a page fault is generated if not set
#define PD_RDONLY (1L << 7)    // 0 for read-write, 1 for read-only.
#define PD_UK_ACCESS (1L << 6) // 0 for only kernel access, 1 for user/kernel access.

#define PERIPHERAL_START 0x3C000000L
#define PERIPHERAL_END 0x3F000000L
#define USER_DATA_BASE 0x00000000L
#define USER_STACK_BASE 0x0000fffffffff000L
#define USER_SIGNAL_WRAPPER_VA 0x0000FFFFFFFAF000L //<-------------------------------------
#define USER_EXEC_WRAPPER_VA 0x0000FFFFFFFBF000L //<-------------------------------------

#define MMU_PGD_BASE 0x1000L
#define MMU_PGD_ADDR (MMU_PGD_BASE + 0x0000L)
#define MMU_PUD_ADDR (MMU_PGD_BASE + 0x1000L)
#define MMU_PTE_ADDR (MMU_PGD_BASE + 0x2000L)

// Used for EL1
#define BOOT_PGD_ATTR (PD_TABLE)
#define BOOT_PUD_ATTR (PD_TABLE | PD_ACCESS)
#define BOOT_PTE_ATTR_nGnRnE (PD_BLOCK | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_UNX | PD_KNX | PD_UK_ACCESS) // p.17
#define BOOT_PTE_ATTR_NOCACHE (PD_BLOCK | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2))

#ifndef __ASSEMBLER__

#include "sched.h"
#include "exception.h"
#include "u_list.h"
#include "stddef.h"


#define PERMISSION_INVAILD(userId,VMA_Permission) (userId&~VMA_Permission)
#define DUMP_NAME(number, name) \
    case number:                \
        uart_sendlinek(name);   \
        uart_sendlinek("\n");   \
        break;

typedef enum
{
    UNKNOW_AREA = -1,
    USER_DATA,
    USER_STACK,
    PERIPHERAL,
    USER_SIGNAL_WRAPPER,
    USER_EXEC_WRAPPER
} vma_name_type;

typedef enum
{
    PGD,
    PUD,
    PMD,
    PTE,
} pagetable_type;

typedef struct vm_area_struct
{
    list_head_t listhead;
    unsigned long virt_addr;
    unsigned long phys_addr;
    unsigned long area_size;
    unsigned long rwx; // 1, 2, 4
    int is_alloced;
    vma_name_type name;
} vm_area_struct_t;

void *set_2M_kernel_mmu(void *x0);
void map_one_page(size_t *pgd_p, size_t va, size_t pa, size_t flag);
void mmu_add_vma(struct thread *t, size_t va, size_t size, size_t pa, size_t rwx, int is_alloced, vma_name_type name);
void mmu_del_vma(struct thread *t);
void mmu_free_page_tables(size_t *page_table, int level);
// void mmu_set_PTE_readonly(size_t *page_table, int level);
// void mmu_pagetable_copy(unsigned long *dst_page_table, unsigned long *src_page_table, int level);
void mmu_memfail_abort_handle(esr_el1_t *esr_el1);

vm_area_struct_t *check_vma_overlap(thread_t *t,unsigned long user_va, unsigned long size);
int check_permission(int userId, int requiredPermission);
void dump_vma();
void dump_pagetable(unsigned long user_va, unsigned long pa);

#endif //__ASSEMBLER__

#endif /* _MMU_H_ */
