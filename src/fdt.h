#ifndef _DEF_FDT
#define _DEF_FDT

#include "type.h"

/**
 * FDT definition (flattened device tree)
*/

typedef int(*fdt_callback_t) (int, int);
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

// FDT node
// Linux fdt node: https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/dtc.h#L153
typedef struct fdt_node {
    char *name;
    struct fdt_property *proplist;

    struct fdt_node *parent;
    struct fdt_node *children;
    struct fdt_node *next_sibling;
} fdt_node_t;

typedef struct fdt_property {
    char *name;
    int length;
    char *value;
    struct fdt_property *next;
} fdt_property_t;

typedef struct {
    uint32_t len;
    uint32_t nameoff;
} fdt_propmeta_t;

// https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/livetree.c#L100
fdt_node_t *build_node(fdt_property_t *, fdt_node_t *);

// https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/flattree.c#L745
fdt_node_t *unflatten_tree(uint32_t *);

// https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/livetree.c#L249
void add_property(fdt_node_t *, fdt_property_t *);

// https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/livetree.c#L281
void add_child(fdt_node_t *, fdt_node_t *);

int read_string(fdt_node_t *, uint32_t *);
int read_property(fdt_node_t *, uint32_t *);

#define BLOB_ADVANCE(b, e) (b = (uint32_t *) ((char *) (b) + (e)))

fdt_property_t *init_property_t();

#endif
