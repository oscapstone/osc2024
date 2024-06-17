#ifndef _RPI_MMU_H_
#define _RPI_MMU_H_

#define PHYS_TO_VIRT(x) (x + 0xffff000000000000)
#define VIRT_TO_PHYS(x) (x - 0xffff000000000000)
#define ENTRY_ADDR_MASK 0x0000fffffffff000L

#define PHYS_TO_KERNEL_VIRT(x) (((unsigned long)(x)) | 0xffff000000000000)
#define KERNEL_VIRT_TO_PHYS(x) (((unsigned long)(x)) & ~0xffff000000000000)

// e.g. size=0x13200, alignment=0x1000 -> 0x14000
#define ALIGN_UP(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))
// e.g. size=0x13200, alignment=0x1000 -> 0x13000
#define ALIGN_DOWN(size, alignment) ((size) & ~((alignment) - 1))

#define IS_NOT_ALIGN(ptr, alignment) (((unsigned long)ptr & ((alignment) - 1)) != 0)

#endif /*_RPI_MMU_H_ */
