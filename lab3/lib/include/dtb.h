#ifndef __DTB_H__
#define __DTB_H__

#include "stdint.h"
// #include "string.h"

// five token types
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

typedef struct fdt_header {
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
} fdt_header;
typedef void (*dtb_callback_t)(uint32_t token_type, char *name, char *data);

void dtb_init();
void dtb_parser(dtb_callback_t callback);
void dtb_get_initrd_callback(uint32_t token_type, char *name, char *data);
void dtb_show_callback(uint32_t token_type, char *name, char *data);

#endif