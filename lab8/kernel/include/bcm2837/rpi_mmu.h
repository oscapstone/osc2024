#ifndef	_RPI_MMU_H_
#define	_RPI_MMU_H_

#define PHYS_TO_VIRT(x)   (x + 0xffff000000000000)
#define VIRT_TO_PHYS(x)   (x - 0xffff000000000000)
#define ENTRY_ADDR_MASK   0xfffffffff000L

#endif  /*_RPI_MMU_H_ */
