#include "memblock.h"
#include "kernel.h"
#include "uart.h"
#include "mbox.h"
#include "dtb.h"
#include "initrd.h"

#define INIT_MEMLBOCK_REGIONS	(128)

#define __for_each_mem_range(i, type_a, type_b,		\
			   p_start, p_end)			\
	for (i = 0, __next_mem_range(&i, type_a, type_b,	\
				     p_start, p_end);		\
	     i != (u64)ULLONG_MAX;					\
	     __next_mem_range(&i, type_a, type_b,		\
			      p_start, p_end))

#define for_each_free_mem_range(i, p_start, p_end)		\
	__for_each_mem_range(i, &memblock.memory, &memblock.reserved,	\
			     p_start, p_end)

extern unsigned char _end; // defined in linker.ld, kernel end address.
unsigned int phys_addr_max = (0x3F000000); // the address of the end of the physical memory

static struct memblock_region memblock_memory_init_regions[INIT_MEMLBOCK_REGIONS];
static struct memblock_region memblock_reserved_init_regions[INIT_MEMLBOCK_REGIONS];

struct memblock memblock = {
    .memory.regions = memblock_memory_init_regions,
    .memory.cnt = 1, /* empty dummy entry */
    .memory.max = INIT_MEMLBOCK_REGIONS,

    .reserved.regions = memblock_reserved_init_regions,
    .reserved.cnt = 1, /* empty dummy entry */
    .reserved.max = INIT_MEMLBOCK_REGIONS,
};

static void memblock_set_current_limit(phys_addr_t limit)
{
	memblock.current_limit = limit;
}

void __next_mem_range(u64 *idx, struct memblock_type *type_a,
		      struct memblock_type *type_b, phys_addr_t *out_start,
		      phys_addr_t *out_end)
{
    int idx_a = *idx & 0xffffffff;
    int idx_b = *idx >> 32;

    for (; idx_a < type_a->cnt; idx_a++) {
        struct memblock_region *m = &type_a->regions[idx_a];

        phys_addr_t m_start = m->base;
        phys_addr_t m_end = m_start + m->size;

        if (!type_b) {
            if (out_start)
                *out_start = m_start;
            if (out_end)
                *out_end = m_end;
            idx_a++;
            *idx = (u32) idx_a | ((u64) idx_b << 32);
            return;
        }

        /* Scan areas before each reservation region. */
        for (; idx_b < type_b->cnt + 1; idx_b++) {
            struct memblock_region *r;
            phys_addr_t r_start;
            phys_addr_t r_end;

            r = &type_b->regions[idx_b];
            r_start = idx_b ? r[-1].base + r[-1].size : 0; // r[-1].base + r[-1].size is the end of the previous region
            r_end = idx_b < type_b->cnt ? r->base : phys_addr_max; // r->base is the start of the current region

            if (r_start >= m_end)
                break;
            
            if (m_start < r_end) { // overlap
                if (out_start)
                    *out_start = max(m_start, r_start); // max(m_start, r_start) is the start of the overlap region
                if (out_end)
                    *out_end = min(m_end, r_end); // min(m_end, r_end) is the end of the overlap region
                if (m_end <= r_end) // m is covered by r
                    idx_a++;
                else
                    idx_b++;
                *idx = (u32) idx_a | ((u64) idx_b << 32);
                return;
            }
        }
    }
}

/**
 * Return address will be aligned with `align`.
 * ref: `__memblock_find_range_bottom_up()` in linux */
static phys_addr_t memblock_find_in_range(phys_addr_t size,
                    phys_addr_t align, phys_addr_t start, phys_addr_t end)
{
    phys_addr_t this_start, this_end, cand = 0;
    u64 i;

    for_each_free_mem_range(i, &this_start, &this_end) {
        /* clamp(val, lo, hi): 全部轉型成 typeof(val), 然後 min(val, lo)，在跟 max 做 min。基本上就是限制 val 一定要在 lo 跟 hi 之間 */
        this_start = clamp(this_start, start, end);
        this_end = clamp(this_end, start, end);
        cand = ALIGN(this_start, align);
        if (cand < this_end && this_end - cand >= size)
            return cand;
    }
    return 0;
}

static void memblock_insert_region(struct memblock_type *type,
						   int idx, phys_addr_t base, phys_addr_t size)
{
	struct memblock_region *rgn = &type->regions[idx];

	memmove(rgn + 1, rgn, (type->cnt - idx) * sizeof(*rgn));
	rgn->base = base;
	rgn->size = size;
	type->cnt++;
	type->total_size += size;
}

static int memblock_double_array(struct memblock_type *type,
                 phys_addr_t base, phys_addr_t size)
{
    /* Not implemented for now, stuck here */
    while (1);
    return 0;
}

static void memblock_merge_regions(struct memblock_type *type)
{
    int i = 0;

    while (i < type->cnt - 1) {
        struct memblock_region *this = &type->regions[i];
        struct memblock_region *next = &type->regions[i + 1];

        if (this->base + this->size != next->base) {
            i++;
            continue;
        }

        this->size += next->size;
        memmove(next, next + 1, (type->cnt - (i + 2)) * sizeof(*next));
        type->cnt--;
    }
}

/**
 * Add a range into memblock_type regions. size should be aligned before calling this function.
 */
static int memblock_add_range(struct memblock_type *type,
                phys_addr_t base, phys_addr_t size)
{
    phys_addr_t obase = base;
    phys_addr_t end = base + size;
    bool insert = false;
    int idx, nr_new;
    struct memblock_region *rgn;

    if (!size)
        return 0;
    
    if (type->regions[0].size == 0) {
        type->regions[0].base = base;
        type->regions[0].size = size;
        type->total_size = size;
        return 0;
    }

    /** The worst case is that we insert a big range which covers all the empty space bwtween all the served regions.
     * So before we merge regions, we have to make sure that the array is big enough to hold all the regions.
     * And big enough means cnt * 2 + 1 >= max.
     * e.g.   |   10byte   |  10byte  |   10byte   |  10 byte  |   10 byte   |  10 byte  |
     *        |            | reserved |            |  reserved |             |  reserved |
     *  If we want to insert a 30 byte region, we will insert three 10 byte regions in this case.
     */
    if (type->cnt * 2 + 1 < type->max)
        insert = true;

repeat:
    base = obase;
    nr_new = 0;

    /* Iterate every memblock_region of memblock_type type */
    /* Some thing weird here (trying to reserve memory)*/
    for_each_memblock_type(idx, type, rgn) {
        phys_addr_t rbase = rgn->base;
        phys_addr_t rend = rbase + rgn->size;

        if (rbase >= end) // We can insert the new region here
            break;
        if (rend <= base) // try next reserved region
            continue;
        
        if (rbase > base) {
            nr_new++;
            if (insert)
                memblock_insert_region(type, idx++, base, rbase - base);
        }
        base = min(rend, end);
    }
    
    /* insert remaining portion */
    if (base < end) {
        nr_new++;
        if (insert)
            memblock_insert_region(type, idx, base, end - base);
    }

    if (!nr_new) // no new region added
        return 0;

    /* If this was the first round, resize array and repeat for actual insertions; otherwise, merge and return.*/
    if (!insert) { // 
        while (type->cnt + nr_new > type->max) {
            /* There is no enough regions array, we have to double the array */
            if (memblock_double_array(type, obase, size) < 0)
                return -ENOMEM;
        }
        insert = true;
        goto repeat;
    } else {
        memblock_merge_regions(type);
        return 0;
    }
}

/**
 * Linux use `base` and `size` to compute `end`. size should be aligned before calling this function.
 * Or size will align to sizeof(unsigned long) by default.
 */
int memblock_reserve(phys_addr_t base, phys_addr_t size)
{
    return memblock_add_range(&memblock.reserved, base, ALIGN(size, sizeof(unsigned long)));
}


/* return address will be aligned with `align` */
phys_addr_t memblock_phys_alloc_range(unsigned long size, unsigned long align,
                    unsigned long start, unsigned long end)
{
    phys_addr_t ret = 0;
    if (end == MEMBLOCK_ALLOC_ACCESSIBLE)
        end = phys_addr_max;

    ret = memblock_find_in_range(size, align, start, end);
    if (ret)
        memblock_reserve(ret, size);
    return ret;
}

/* Align 8 bytes, allocate from all managable memory region */
phys_addr_t memblock_phys_alloc(phys_addr_t size)
{
	return memblock_phys_alloc_range(size, sizeof(unsigned long), 0,
					 MEMBLOCK_ALLOC_ACCESSIBLE);
}

void memblock_init(void)
{
    /* Get memory information dynamically. */
    unsigned int base, size;
    __get_memory_info(&base, &size);

    /* Update the `phys_addr_max` */
    phys_addr_max = base + size;

    /* Set the current_limit */
    memblock_set_current_limit(phys_addr_max);

    /* Add the whole memory into memblock_type memory */
    memblock_add_range(&memblock.memory, base, size);

    /* Reserve the kernel memory */
    memblock_reserve(0, (unsigned long) &_end - 0);

    /* Reserve the initrd memory (cpio). */
    initrd_reserve_memory();

    /* Reserve the device tree memory. */
    fdt_reserve_memory();
}

void print_memblock_info(void)
{
    printf("\n===============\nMemory block information:\n");
    printf("Memory:\n");
    for (int i = 0; i < memblock.memory.cnt; i++) {
        // uart_puts("    ");
        printf("    regions [%d]: ", i);
        printf("%x - %x\n", memblock.memory.regions[i].base, \
                memblock.memory.regions[i].base + memblock.memory.regions[i].size);
    }
    printf("Reserved:\n");
    for (int i = 0; i < memblock.reserved.cnt; i++) {
        printf("    regions [%d]: ", i);
        printf("%x - %x\n", memblock.reserved.regions[i].base, \
                memblock.reserved.regions[i].base + memblock.reserved.regions[i].size);
    }
    uart_puts("===============\n");
}