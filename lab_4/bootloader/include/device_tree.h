#include "stdint.h"


typedef struct fdt_header {
    //  0xd00dfeed (big-endian)
    uint32_t magic;
    uint32_t totalsize;
    // offsets of each block from start
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;

    uint32_t version;
    uint32_t last_comp_version;
    // physical ID of the system’s boot CPU
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} device_tree_header;

#define ALIGN_MEMORY_BLOCK 8

/* All tokens shall be aligned on a 32-bit boundary */
#define ALIGN_STRUCT_BLOCK 4

/* tokens used for structure block */
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009


void header_parser();
