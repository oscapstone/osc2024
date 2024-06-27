#ifndef DTB_H
#define DTB_H

#include <stdint.h>

/* structur of DTB format
 *
 * --------------------------- *
 * |   struct fdt_header     |
 * --------------------------- *               
 * |       (free space)      |
 * --------------------------- *
 * | memory reservation block|
 * --------------------------- *
 * |       (free space)      |
 * --------------------------- *
 * |     structure block     |
 * --------------------------- *
 * |       (free space)      |
 * --------------------------- *
 * |      strings block      |
 * --------------------------- *
 * |       (free space)      |
 * --------------------------- *        
 */

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

#define FDT_TRAVERSE_CORRECT        0
#define FDT_TRAVERSE_HEADER_ERROR   1
#define FDT_TRAVERSE_FORMAT_ERROR   2

typedef struct fdt_header {
    uint32_t magic;                // 0xd00dfeed (big-endian)
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

typedef void (*fdt_callback)(uint32_t token, char *nameptr, char *dataptr, uint32_t v);

uint32_t fdt_traverse(fdt_callback cb, uint64_t dtb_addr);
void get_cpio_addr(uint32_t token, char *nameptr, char *dataptr, uint32_t v);
void get_cpio_end(uint32_t token, char *nameptr, char *dataptr, uint32_t v);
uint32_t set_dtb_end(uint64_t dtb_addr);

#endif