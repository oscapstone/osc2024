#ifndef _MMU_H_
#define _MMU_H_

#include "stddef.h"
// tcr_el1: The control register for stage 1 of the EL1&0 translation regime.
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16)) // T0SZ[5:0]   The size offset for ttbr0_el1 is 2**(64-T0SZ): 0x0000_0000_0000_0000 <- 0x0000_FFFF_FFFF_FFFF
#define TCR_CONFIG_4KB ((0b00 << 14) | (0b10 << 30))                   // T1SZ[21:16] The size offset for ttbr1_el1 is 2**(64-T1SZ): 0xFFFF_0000_0000_0000 -> 0xFFFF_FFFF_FFFF_FFFF
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)  // TG0[15:14]  Granule size for the TTBR0_EL1: 0b00 = 4KB // TG1[31:30]  Granule size for the TTBR1_EL1: 0b10 = 4KB

#define MAIR_DEVICE_nGnRnE 0b00000000  // ((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))
#define MAIR_NORMAL_NOCACHE 0b01000100 // mair_el1: Provides the memory attribute encodings corresponding to the possible AttrIndx values for stage 1 translations at EL1.
#define MAIR_IDX_DEVICE_nGnRnE 0       // ATTR0[7:0]: 0b0000dd00 Device memory,   dd = 0b00   Device-nGnRnE memory
#define MAIR_IDX_NORMAL_NOCACHE 1      // ATTR1[14:8] 0booooiiii Normal memory, oooo = 0b0100 Outer Non-cacheable, iiii = 0b0100 Inner Non-cacheable

#define PD_TABLE 0b11L         // Table Entry Armv8_a_address_translation p.14
#define PD_BLOCK 0b01L         // Block Entry
#define PD_UNX (1L << 54)      // non-executable page frame for EL0 if set
#define PD_KNX (1L << 53)      // non-executable page frame for EL1 if set
#define PD_ACCESS (1L << 10)   // a page fault is generated if not set
#define PD_RDONLY (1L << 7)    // 0 for read-write, 1 for read-only.
#define PD_UK_ACCESS (1L << 6) // 0 for only kernel access, 1 for user/kernel access.

#define PERIPHERAL_START 0x3C000000L
#define PERIPHERAL_END 0x40000000L
#define USER_CODE_BASE 0x0000000000000000L
#define USER_STACK_BASE 0x0000FFFFFFFFF000L // top of stack
#define USER_SIGNAL_WRAPPER_VA 0x0000FFFFFFFE0000L
#define USER_RUN_USER_TASK_WRAPPER_VA 0x0000FFFFFFFD0000L

#define MMU_PGD_BASE 0x2000L
#define MMU_PGD_ADDR (MMU_PGD_BASE + 0x0000L)
#define MMU_PUD_ADDR (MMU_PGD_BASE + 0x1000L)
#define MMU_PTE_ADDR (MMU_PGD_BASE + 0x2000L)

// Used for EL1
#define BOOT_PGD_ATTR (PD_TABLE)
#define BOOT_PUD_ATTR (PD_TABLE | PD_ACCESS)
#define BOOT_PTE_ATTR_nGnRnE (PD_BLOCK | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_UNX | PD_KNX | PD_UK_ACCESS) // p.17
#define BOOT_PTE_ATTR_NOCACHE (PD_BLOCK | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2))

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define VM_NONE 0x00000000
#define VM_READ 0x00000001
#define VM_WRITE 0x00000002
#define VM_EXEC 0x00000004
#define VM_SHARED 0x00000008
#define VM_GROWSDOWN 0x00000100

// 假設頁表的基本條件
#define PAGE_OFFSET_BITS 12
#define PTE_INDEX_BITS 9
#define PMD_INDEX_BITS 9
#define PUD_INDEX_BITS 9
#define PGD_INDEX_BITS 9

// 取得頁表索引的宏
#define PGD_INDEX(addr) (((addr) >> (PAGE_OFFSET_BITS + PTE_INDEX_BITS + PMD_INDEX_BITS + PUD_INDEX_BITS)) & ((1ULL << PGD_INDEX_BITS) - 1))
#define PUD_INDEX(addr) (((addr) >> (PAGE_OFFSET_BITS + PTE_INDEX_BITS + PMD_INDEX_BITS)) & ((1ULL << PUD_INDEX_BITS) - 1))
#define PMD_INDEX(addr) (((addr) >> (PAGE_OFFSET_BITS + PTE_INDEX_BITS)) & ((1ULL << PMD_INDEX_BITS) - 1))
#define PTE_INDEX(addr) (((addr) >> PAGE_OFFSET_BITS) & ((1ULL << PTE_INDEX_BITS) - 1))

// 物理地址轉換宏
#define VIRT_TO_PHYS(phys_pgd_ptr, virt_addr) ({                                                           \
    uint64_t *pgd = (uint64_t *)(PHYS_TO_KERNEL_VIRT(phys_pgd_ptr));                                       \
    uint64_t *pud = (uint64_t *)(pgd[PGD_INDEX(virt_addr)] & ENTRY_ADDR_MASK);                             \
    uint64_t *pmd = (uint64_t *)(pud[PUD_INDEX(virt_addr)] & ENTRY_ADDR_MASK);                             \
    uint64_t *pte = (uint64_t *)(pmd[PMD_INDEX(virt_addr)] & ENTRY_ADDR_MASK);                             \
    uint64_t phys_addr = (pte[PTE_INDEX(virt_addr)] & ENTRY_ADDR_MASK) | ((virt_addr) & ~ENTRY_ADDR_MASK); \
    phys_addr;                                                                                             \
})

#ifndef __ASSEMBLER__

#include "sched.h"
#include "exception.h"

/**
 * @brief memory area
 *
 * @param start: start address (included)
 * @param end: end address (excluded)
 */
typedef struct area
{
    size_t start;
    size_t end;
} area_t;

/**
 * @brief virtual memory area
 *
 * @param listhead
 * @param virt_addr_area vma start address in user space virtual address
 * @param phys_addr_area vma start address in physical address
 * @param rwx 3-bit: read/write/execute
 * @param need_to_free when free vma, free the physical page
 */
typedef struct vm_area_struct
{
    list_head_t listhead;
    area_t virt_addr_area;
    area_t phys_addr_area;
    uint64_t vm_page_prot;
    uint64_t vm_flags;
    uint8_t need_to_free;
    void *vm_file;
    char *name;
} vm_area_struct_t;

typedef enum PAGE_TABLE_LEVEL
{
    LEVEL_PGD = 0,
    LEVEL_PUD = 1,
    LEVEL_PMD = 2,
    LEVEL_PTE = 3,
} PAGE_TABLE_LEVEL;

typedef struct thread_struct thread_t;

#define VMA_COPY(dest_thread, src_thread)                                                      \
    do                                                                                         \
    {                                                                                          \
        DEBUG("Copy vma_list from %d to %d\n", src_thread->pid, dest_thread->pid);             \
        dest_thread->vma_list = (vm_area_struct_t *)kmalloc(sizeof(vm_area_struct_t));         \
        DEBUG("kmalloc vma_list 0x%x\n", dest_thread->vma_list);                               \
        INIT_LIST_HEAD((list_head_t *)dest_thread->vma_list);                                  \
        DEBUG("INIT_LIST_HEAD vma_list\n");                                                    \
        list_head_t *curr;                                                                     \
        list_for_each(curr, (list_head_t *)src_thread->vma_list)                               \
        {                                                                                      \
            vm_area_struct_t *new_vma = (vm_area_struct_t *)kmalloc(sizeof(vm_area_struct_t)); \
            DEBUG("kmalloc new_vma 0x%x\n", new_vma);                                          \
            new_vma = (vm_area_struct_t *)curr;                                                \
            DEBUG("Copy vma 0x%x -> 0x%x\n", curr, new_vma);                                   \
            list_add_tail((list_head_t *)new_vma, (list_head_t *)dest_thread->vma_list);       \
        }                                                                                      \
    } while (0)

void *set_2M_kernel_mmu(void *x0);
void map_one_page(size_t *virt_pgd_p, size_t va, size_t pa, size_t flag);
// void mmu_add_vma(thread_t *t, char *name, size_t va, size_t size, char *file, uint64_t vm_page_prot, uint64_t vm_flags, uint8_t need_to_free)
void mmu_add_vma(thread_t *t, char *name, size_t va, size_t size, size_t pa, uint64_t vm_page_prot, uint64_t vm_flags, uint8_t need_to_free);
void mmu_free_all_vma(thread_t *t);
void mmu_clean_page_tables(size_t *page_table, PAGE_TABLE_LEVEL level);
void mmu_memfail_abort_handle(esr_el1_t *esr_el1);
void dump_vma(thread_t *t);

#endif //__ASSEMBLER__

#endif /* _MMU_H_ */
