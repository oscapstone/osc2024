#pragma once

#include "arm/arm.h"
#include "proc/task.h"

#define MMU_PERIPHERAL_START        0x3c000000L
#define MMU_PERIPHERAL_END          0x3f000000L
#define MMU_USER_KERNEL_BASE        0x00000000L
#define MMU_USER_STACK_BASE         0xfffffffff000L
#define MMU_SINGAL_ENTRY_BASE       (MMU_USER_STACK_BASE - TASK_STACK_SIZE - 2 * PD_PAGE_SIZE)    // only one page

#define MMU_PHYS_TO_VIRT(x)   (x + 0xffff000000000000L)
#define MMU_VIRT_TO_PHYS(x)   (x - 0xffff000000000000L)
/**
 * For ARM MMU
 * 
 * page descriptor format (D5.4.2 armv8 pg. 1775)
 *                              Descriptor format
 * +------------------------------------------------------------------------------------------+
 * | Upper attributes | Address (bits 47:12) | Lower attributes | Block/table bit | Valid bit |
 * +------------------------------------------------------------------------------------------+
 * 63                 47                     |11               2|                1|          0|
 * Bit 0 This bit must be set to 1 for all valid descriptors. If MMU encounter non-valid descriptor during translation process a synchronous exception is generated. The kernel then should handle this exception, allocate a new page and prepare a correct descriptor (We will look in details on how this works a little bit later)
 * Bit 1 This bit indicates whether the current descriptor points to a next page table in the hierarchy (we call such descriptor a "table descriptor") or it points instead to a physical page or a section (such descriptors are called "block descriptors").
 * Bits [11:2] Those bits are ignored for table descriptors. For block descriptors they contain some attributes that control, for example, whether the mapped page is cachable, executable, etc.
 *
 *     upper attribute (stage 1)
 *     +--------------------------------------------------------------------+
 *     | Ignored | Reserved for software use | UXN or XN | PXN | Contiguous |
 *     +--------------------------------------------------------------------+
 *     |63    59 | 58                     55 | 54        | 53  | 52         |
 *      UXN or XN
 *          in EL0, 1 => UXN (Unprivileged execute never)
 *          是否可以執行特權指令?(關中斷?之類的)
 *          0: 可以執行, 1: 不可執行
 *      PXN
 *          Privileged execute never
 *          
 *
 *     lower attribute (stage 1)
 *     +-------------------------------------------+
 *     | nG | ACCESS | SHARE | AP | NS | MAIR IDX |
 *     +-------------------------------------------+
 *     |11  | 10     |9     8|7  6|5   |4        2|
 * 
 *      AP[7:6] Data access premission
 *             | EL0            | EL1/2/3
 *          00 | None           | RW
 *          01 | RW             | RW
 *          10 | None           | Read-only
 *          11 | Read-only      | Read-only
 * 
 * Bits [47:12]. This is the place where the address that a descriptor points to is stored. As I mentioned previously, only bits [47:12] of the address need to be stored, because all other bits are always 0.
 * Bits [63:48] Another set of attributes.
 * 
 * Virtual address format
 * each level contain 9 bit
 * for two level paging the page size is 1GB [29 ~ 0] = 2^30
 * for three level paging the page size is 2MB [20 ~ 0] = 2^21
 * for four level paging the page size is 4KB [11 ~ 0] = 2^12
 * 4kb in memory is 0x1000
*/

#define MMU_UNX              (1L << 54)         // non-executable page frame for EL0 if set
#define MMU_PXN              (1L << 53)         // non-executable page frame for EL1 if set

// filter for page
#define PD_PAGE_MASK    	0x00000000fffff000
// [0, 2]
#define PD_TABLE            0x3
#define PD_BLOCK            0x1
// access flag in lower attribute
#define PD_ACCESS           (1 << 10)
// kernel pgd attribute
#define PD_KERNEL_PGD_ATTR  PD_TABLE
// kernel pud attribute
#define PD_KERNEL_PUD_ATTR  (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#define MMU_AP_EL0_NONE         (0)
#define MMU_AP_EL0_READ_ONLY    (1L << 7)       // 0 for read-write, 1 for read-only.
#define MMU_AP_EL0_UK_ACCESS   (1L << 6)        // 0 for only kernel access, 1 for user/kernel access.

#define PD_PGD_SHIFT        39
#define PD_PUD_SHIFT        30
#define PD_PMD_SHIFT        21

#define PD_PAGE_SHIFT       12
#define PD_TABLE_SHIFT      9

#define PD_PTRS_PER_TABLE   (1 << PD_TABLE_SHIFT)
// 4k page size
#define PD_PAGE_SIZE        (1 << PD_PAGE_SHIFT)

// can't use 0x0 for entry, the real machine will not work, 我蝦中了
// 2024/5/13 0x0 ~ 0x1000 is for multiboot spin table
// the kernel boot page table entry
#define PD_KERNEL_ENTRY     0x1000
// 
#define PD_FIRST_PUD_ENTRY  0x2000

#define PD_KERNEL_ENTRY_VA ((pd_t *)MMU_PHYS_TO_VIRT(0x1000L))
#define PD_FIRST_PUD_ENTRY_VA ((pd_t *)MMU_PHYS_TO_VIRT(0x2000L))

void setup_kernel_space_mapping();

// for task process
void mmu_map_table_entry(pd_t* pte, U64 v_addr, U64 p_addr, U64 flags);
U64 mmu_get_pte(TASK* task, U64 v_addr);
/**
 * Assign a page to this task virtual address
 * This function create each level table for the target page (save in task kernel pages)
 * the target page are store in vma struct
 * @param task
 *      the task this virtual space use
 * @param v_addr
 *      the virtual address this page will assign to
 * @param page
 *      the page to be insert
 * @param flags
 *      page descriptor flags
 * @return user page
 *      the allocated page physical address
*/
USER_PAGE_INFO* mmu_map_page(TASK* task, U64 v_addr, U64 page, U64 flags);
/***
 * Initialize the stack mapping and the peripherial mapping
*/
void mmu_task_init(TASK* task);
void mmu_map_io(TASK* task);

/**
 * Delete all page descriptor and mapped page for this task
*/
void mmu_delete_mm(TASK* task);
void mmu_fork_mm(TASK* src_task, TASK* new_task);

/**
 * Transfer virtual addres to physical address according to current process page table
*/
void* mmu_va2pa(UPTR v_addr);

/**
 * Allocate a virtual area for this process
 * @param task
 *      this process task structure
 * @param v_start
 *      the virtual address starting point
 * @param page_count
 *      how many contiguous page this area have in virtual space
 * @param flags
 *      VMA_FLAGS
*/
//int mmu_vma_alloc(TASK *task, UPTR v_start, U32 page_count, U32 flags);

// the handler for memory failed
void mmu_memfail_handler(U64 esr);