#ifndef _DEVTREE_H
#define _DEVTREE_H

#include <lib/stdlib.h>
#include <lib/utils.h>

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

void fdt_traverse(void (*callback)(void *));
uint64_t fdt_get_initrd_start();
uint64_t fdt_get_initrd_end();

/**
 * @brief Convert a 4-byte big-endian sequence to little-endian.
 *
 * @param s: big-endian sequence
 * @return little-endian sequence
 */
static uint32_t be2le(const void *s);

#endif
