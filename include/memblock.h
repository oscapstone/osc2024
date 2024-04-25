#ifndef __MEMBLOCK_H__
#define __MEMBLOCK_H__

#include "types.h"
#include "stdint.h"

#define MEMBLOCK_ALLOC_ACCESSIBLE	0

#define for_each_memblock_type(i, memblock_type, rgn) \
    for (i = 0, rgn = &((memblock_type)->regions[0]); i < (memblock_type)->cnt; i++, rgn = &(memblock_type)->regions[i])

enum memblock_flags {
	MEMBLOCK_NONE		= 0x0,	/* No special request */
	MEMBLOCK_HOTPLUG	= 0x1,	/* hotpluggable region */
	MEMBLOCK_MIRROR		= 0x2,	/* mirrored region */
	MEMBLOCK_NOMAP		= 0x4,	/* don't add to kernel direct mapping */
	MEMBLOCK_DRIVER_MANAGED = 0x8,	/* always detected via a driver */
};

struct memblock_region {
    unsigned long base;
    unsigned long size;
    // enum memblock_flags flags;
};

struct memblock_type {
    unsigned long cnt; // region count
    unsigned long max; // maximum region number
    unsigned long total_size;

    struct memblock_region *regions;
    // char *name;
};

struct memblock {
    phys_addr_t current_limit;
    struct memblock_type memory;
    struct memblock_type reserved;
};

extern struct memblock memblock;

int memblock_reserve(phys_addr_t base, phys_addr_t end);
phys_addr_t memblock_phys_alloc(phys_addr_t size);

void print_memblock_info(void);
void memblock_init(void);

#endif // __MEMBLOCK_H__