#ifndef _RPI_MMU_H_
#define _RPI_MMU_H_
#include "stdint.h"

#define _PHYS_TO_KERNEL_VIRT(x) (x + 0xFFFF000000000000L)
#define _KERNEL_VIRT_TO_PHYS(x) (x - 0xFFFF000000000000L)

#define PHYS_TO_KERNEL_VIRT(x) ((uint64_t)x | 0xFFFF000000000000L)
#define KERNEL_VIRT_TO_PHYS(x) ((uint64_t)x & ~0xFFFF000000000000L)
#define ENTRY_ADDR_MASK 0xfffffffff000L

#endif /*_RPI_MMU_H_ */
