#ifndef _DT_H_
#define _DT_H_

#include "alloc.h"

#define ALIGN_MEMORY_BLOCK 8
#define ALIGN_STRUCT_BLOCK 4
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef struct _header_block {
    uint32_t magic;                 // Contain value 0xd00dfeed(big-endian)
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} DTBHeader;

extern DTBHeader myHeader;

// Define a callback function type for processing nodes.
typedef void (*fdt_node_callback_t)(int type, const char* name, void* data, uintptr_t cur_ptr);
int fdt_traverse(fdt_node_callback_t callback, uintptr_t dtb_address);
int parse_struct(fdt_node_callback_t callback, uintptr_t dtb_start_addr);
void print_tree(int type, const char* name, void* data, uintptr_t cur_ptr);
void get_initrd_address(int type, const char* name, void* data, uintptr_t cur_ptr);

#endif