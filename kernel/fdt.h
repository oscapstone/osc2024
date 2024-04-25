#ifndef _DEF_FDT
#define _DEF_FDT

#include "type.h"

/**
 * FDT definition (flattened device tree)
*/
typedef int(*fdt_callback_t) (int);
void fdt_traverse(fdt_callback_t);

typedef struct {
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
} fdt_header_t;


// FDT tokens
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef struct {
    uint32_t len;
    uint32_t nameoff;
} fdt_propmeta_t;

typedef struct {
    uint32_t *base;
    uint32_t *ptr;
} blob_t;

// https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/flattree.c#L745
void unflatten_tree(blob_t *);

#define BLOB_ADVANCE(b, e) (b->ptr = (uint32_t *) ((char *) (b->ptr) + (e)))

#endif
