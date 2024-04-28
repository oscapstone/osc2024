#pragma once

/**
 * For ARM MMU
 * 
 * page descriptor format
 *                              Descriptor format
 * +------------------------------------------------------------------------------------------+
 * | Upper attributes | Address (bits 47:12) | Lower attributes | Block/table bit | Valid bit |
 * +------------------------------------------------------------------------------------------+
 * 63                 47                     |11               2|                1|          0|
 * Bit 0 This bit must be set to 1 for all valid descriptors. If MMU encounter non-valid descriptor during translation process a synchronous exception is generated. The kernel then should handle this exception, allocate a new page and prepare a correct descriptor (We will look in details on how this works a little bit later)
 * Bit 1 This bit indicates whether the current descriptor points to a next page table in the hierarchy (we call such descriptor a "table descriptor") or it points instead to a physical page or a section (such descriptors are called "block descriptors").
 * Bits [11:2] Those bits are ignored for table descriptors. For block descriptors they contain some attributes that control, for example, whether the mapped page is cachable, executable, etc.
 *     block attribute (stage 1)
 *     +-------------------------------------------+
 *     | nG | ACCESS | SHARE | AP | NS | MAIR IDX |
 *     +-------------------------------------------+
 *     |11  | 10     |9     8|7  6|5   |4        2|
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

// [0, 2]
#define PD_TABLE            0x3
#define PD_BLOCK            0x1
// access flag in lower attribute
#define PD_ACCESS           (1 << 10)
// kernel pgd attribute
#define PD_KERNEL_PGD_ATTR  PD_TABLE
// kernel pud attribute
#define PD_KERNEL_PUD_ATTR  (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#define PD_PGD_SHIFT        39
#define PD_PUD_SHIFT        30

#define PD_PAGE_SHIFT       12
#define PD_TABLE_SHIFT      9

#define PD_PTRS_PER_TABLE   (1 << PD_TABLE_SHIFT)
// 4k page size
#define PD_PAGE_SIZE        (1 << PD_PAGE_SHIFT)

// can't use 0x0 for entry, the real machine will not work, 我蝦中了
// the kernel boot page table entry
#define PD_KERNEL_ENTRY     0x1000
// 
#define PD_FIRST_PUD_ENTRY  0x2000