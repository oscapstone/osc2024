#ifndef __MM_H__
#define __MM_H__

/* For buddy system usage */
#define PAGE_SHIFT                      (12)
#define MAX_ORDER                       (10)
#define NR_PAGES                        (nr_pages)

/* For kernel virtual memory usage */
#define KERNEL_VADDR_BASE               (0xffff000000000000UL)

/* For peripheral address */
#define PERIPHERAL_BASE                 (0x3c000000UL)
#define PERIPHERAL_SIZE                 (0x01000000UL)

/* Paging is configured by TCR. The following basic configuration is used in lab5 */
#define TCR_T0SZ_48bit                  ((64 - 48) << 0)
#define TCR_T1SZ_48bit                  ((64 - 48) << 16)
#define TCR_CONFIG_REGION_48bit         (TCR_T0SZ_48bit | TCR_T1SZ_48bit)

#define TCR_TG0_4KB                     (0b00 << 14)
#define TCR_TG1_4KB                     (0b10 << 30)
#define TCR_CONFIG_4KB                  (TCR_TG0_4KB | TCR_TG1_4KB)

#define TCR_IRGN1_NC                    (0b00 << 24) // No cacheable
#define TCR_ORGN1_NC                    (0b00 << 26) // No cacheable
#define TCR_CONFIG_CACHE_POLICY_NC      (TCR_IRGN1_NC | TCR_ORGN1_NC) // Non-cacheable

#define TCR_CONFIG_DEFAULT \
    (TCR_CONFIG_REGION_48bit | \
     TCR_CONFIG_4KB | \
     TCR_CONFIG_CACHE_POLICY_NC)

/* MAIR store the memory configuration information */
#define MAIR_DEVICE_nGnRnE              0b00000000
#define MAIR_NORMAL_NOCACHE             0b01000100
#define MAIR_IDX_DEVICE_nGnRnE          0
#define MAIR_IDX_NORMAL_NOCACHE         1

#define MAIR_CONFIG \
    (((MAIR_DEVICE_nGnRnE) << ((MAIR_IDX_DEVICE_nGnRnE) * 8)) | \
     ((MAIR_NORMAL_NOCACHE) << ((MAIR_IDX_NORMAL_NOCACHE) * 8)))

/* Page table entry configuration */
#define PD_TABLE                        0b11
#define PD_BLOCK                        0b01
#define PD_PAGE                         0b11
#define PD_INVALID                      0b00

/* Execute Never bits only for blocks */
#define UXN                             (1UL << 54)
#define PXN                             (1UL << 53)

#define PD_ACCESS                       (1 << 10)

/* Access Permission*/
#define PD_AP_EL0                       (1UL << 6)
#define PD_AP_READOLY                   (1UL << 7)

#define DEVICE_BLOCK_ATTR               (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define NORMAL_BLOCK_ATTR               (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK)
#define DEVICE_PAGE_ATTR                (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_PAGE)
#define NORMAL_PAGE_ATTR                (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE)
#define TABLE_ENTRY                     (PD_TABLE)

#ifndef MMIO_BASE
#define MMIO_BASE                       (KERNEL_VADDR_BASE | (0x3F000000UL))
#endif // MMIO_BASE

#ifndef PAGE_SIZE
#define PAGE_SIZE                       (1 << 12) // 4KB
#endif // PAGE_SIZE

#ifndef ENTRIES_PER_TABLE
#define ENTRIES_PER_TABLE               (512)
#endif // ENTRIES_PER_TABLE

/* We use 1 PGD, 1 PUD and 512 PMD*/
#define MMU_PGD_ADDR                    (0x1000)
#define MMU_PUD_ADDR                    (MMU_PGD_ADDR + 0x1000)
#define MMU_PMD_ADDR                    (MMU_PGD_ADDR + 0x2000)
#define MMU_PTE_ADDR                    (MMU_PGD_ADDR + 0x3000)

/* Used while MMU disabled. */
#define PERIPH_MMIO_BASE                (0x3F000000UL)

/* For page table index */
#define PGD_SHIFT                       (39)
#define PUD_SHIFT                       (30)
#define PMD_SHIFT                       (21)
#define PTE_SHIFT                       (12)
#define PT_INDEX_MASK                   (0x1FF)
#define PGD_INDEX(x)                    (((x) >> PGD_SHIFT) & PT_INDEX_MASK)
#define PUD_INDEX(x)                    (((x) >> PUD_SHIFT) & PT_INDEX_MASK)
#define PMD_INDEX(x)                    (((x) >> PMD_SHIFT) & PT_INDEX_MASK)
#define PTE_INDEX(x)                    (((x) >> PTE_SHIFT) & PT_INDEX_MASK)

/* To get the physical address from page entry*/
#define ENTRY_ADDR_MASK                 (0xfffffffff000UL)

#define ENTRY_IS_TABLE(x)               (((x) & 3) == 3)
#define ENTRY_IS_BLOCK(x)               (((x) & 3) == 1)
#define ENTRY_IS_PAGE(x)                (((x) & 3) == 3)

#define USER_PROG_START                 (0x0)
#define USER_STACK_ADDR                 (0xffffffffb000UL)
#define USER_STACK_TOP                  (0xfffffffff000UL)
#define USER_STACK_SIZE                 (0x4000)

#ifndef __ASSEMBLER__

#include "list.h"
#include "stdint.h"
#include "slab.h"

#define PG_USED                     (1) // page frame used.
#define PG_KERNEL                   (2) // page frame used by kernel.
#define PG_MMIO                     (3) // page frame used by MMIO.
#define PG_AVAIL                    (4) // page frame is free and not in buddy system. e.g. order 2 page fram [0, 1, 2, 3], flags  = [PG_buddy, PG_AVAIL, PG_AVAIL, PG_AVAIL]
#define PG_RESERVED                 (0x80) // page frame is reserved.

#define PG_slab                     (0x20) // page frame is used by slab allocator.
#define PG_buddy                    (0x40) // page frame is free and in buddy system.

struct page {
    unsigned long flags; // page flags: represent the page state, like compound, dirty, etc.
    union {
        struct {
            struct list_head buddy_list; // list_head for buddy system.
            unsigned long private; // used to indicate the order of the page in buddy system.
        };
        struct { /* Used for slab */
            struct list_head slab_list; // list_head for other slabs.
            struct kmem_cache *slab_cache; // point back to the `struct kmem_cache`
            void *freelist; // the address of the free list.

            union { // frozen and inuse are not implemented for now. Just use the counter to represent the number of objects
                unsigned long counters;
                struct {
                    unsigned inuse:16;
                    unsigned objects:15;
                    unsigned frozen:1;
                };
            };
        };
    };
};

struct free_area {
    /* At linux kernel, the free_list is an array for different MIGRATE_TYPE. We don't implement it for now. */
    struct list_head free_list;
    unsigned long nr_free; // number of free pages of this order
};

struct zone {
    unsigned long managed_pages; // number of pages managed by buddy system.
    // unsigned long spanned_pages; // number of pages in the zone. not used for now.

    struct free_area free_area[MAX_ORDER]; // the key data structure for buddy system

    /* active_list and inactive_list is for page reclaim, not used for now */
    // struct list_head active_list;
    // struct list_head inactive_list;
};

/* mem_map points to page frame array. */
extern struct page *mem_map;

/* macro used in physical memory allocator */
#define pfn_to_page(pfn) (mem_map + pfn)
#define page_to_pfn(page) ((unsigned long)((page) - mem_map))

#define pfn_to_phys(pfn) ((unsigned long)(pfn << PAGE_SHIFT))
#define phys_to_pfn(phys) ((unsigned long)phys >> PAGE_SHIFT)

#define page_to_phys(page) (pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys) (pfn_to_page(phys_to_pfn(phys)))

/* Because in kernel space, the virtual address is start from 0xffff.... */
#define phys_to_virt(phys) ((phys) | KERNEL_VADDR_BASE)
#define virt_to_phys(virt) ((virt) & (~KERNEL_VADDR_BASE))

/* Init buddy system and slab allocator. */
void mm_init(void);

/* Allocate page from buddy system. */
struct page *__alloc_pages(unsigned int order);

/* Free a page and merge it into buddy system. Without lock protection for now. */
void free_one_page(struct page *page, unsigned long pfn, unsigned int order);

/* Print out the buddy system information */
void get_buddy_info(void);

/* Reserve memory to mem_block. */
void reserve_mem(unsigned long start, unsigned long end);

/* Kernel memory allocate, return physical address. */
void *kmalloc(unsigned long size);
void kfree(void *obj);

/* Map physical memory to given virtual memory address */
void map_pages(unsigned long *virt_pgd, unsigned long va, unsigned long size, unsigned long pa, unsigned long flags);
/* Walk from pgd, then allocate physical memory if needed. */
void walk(unsigned long *virt_pgd_p, unsigned long va, unsigned long pa, unsigned long flag);
/* remap_pages will allocate a new table to remap the virtual address to physical address */
void remap_pages(unsigned long *virt_pgd, unsigned long va, unsigned long size, unsigned long pa, unsigned long flags);
/* Walk from pgd, always allocate physical memory for page table. */
void rewalk(unsigned long *virt_pgd_p, unsigned long va, unsigned long pa, unsigned long flag);

/* Create empty page table and return kernel virtual address. kmalloc() + memset() to zero. */
unsigned long *create_empty_page_table(void);
/* Simulate walk from pgd, return the physical address. */
unsigned long simulate_walk(unsigned long *virt_pgd, unsigned long va);

#endif // __ASSEMBLER__
#endif // __MM_H__