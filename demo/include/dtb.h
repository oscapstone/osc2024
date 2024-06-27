#ifndef DTB_H
#define DTB_H

#include "../include/my_stdint.h"
#include "../include/uart.h"
#include "../include/my_string.h"


// Define the FDT header structure
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

typedef void (*fdt_callback_t)(const char* name, const char* prop, const void* value, int len);

// FDT tokens
#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

uint32_t fdt32_to_cpu(uint32_t x);
void fdt_traverse(void* fdt, fdt_callback_t callback);
void initramfs_callback(const char* name, const char* prop, const void* value, int len);

#endif