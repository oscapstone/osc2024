#ifndef __DEVTREE_H__
#define __DEVTREE_H__

#include "utils.h"

#define FDT_HEADER_MAGIC    0xd00dfeed 
#define FDT_BEGIN_NODE      0x00000001
#define FDT_END_NODE        0x00000002
#define FDT_PROP            0x00000003
#define FDT_NOP             0x00000004
#define FDT_END             0x00000009

/*
Structure Block
The structure block describes the structure and contents of the devicetree itself. 
It is composed of a sequence of tokens with data
These are organized into a linear tree structure


The structure block is composed of a sequence of pieces, 
each beginning with a token, that is, a big-endian 32-bit integer. 
Some tokens are followed by extra data, the format of which is determined by the token value. 
All tokens shall be aligned on a 32-bit boundary, 
which may require padding bytes (with a value of 0x0) to be inserted after the previous token’s data.

*/

/*
String BLock
The strings block contains strings representing 
all the property names used in the tree
*/
struct fdt_header {
    uint32_t magic;             // 0xd00dfeed big-endian
    uint32_t totalsize;         // total size of devicetree
    uint32_t off_dt_struct;     // offset of structure block from the beginning of the header
    uint32_t off_dt_strings;    // offset of strings block from the beginning of the header
    uint32_t off_mem_rsvmap;    // offset of the memory reservation block from the beginning of the header
    uint32_t version;           // version of the devicetree data structure
    uint32_t last_comp_version; // lowest version of the devicetree data structure with which the version used is backwards compatible
    uint32_t boot_cpuid_phys;   // physical ID of the system's boot CPU
    uint32_t size_dt_strings;   // length of strings block section 
    uint32_t size_dt_struct;    // ength of structure block section 
};

struct fdt_prop {
    uint32_t len;               // length of the property's value (big-endian)
    uint32_t nameoff;           // an offset into the strings block at which the property's name is stored as a null-terminated string (big-endian)
};

void fdt_traverse(void (*callback)(char*, char*, struct fdt_prop*));
uint32_t fdt32_to_cpu(uint32_t fdt_num);//big endian to little endian


#endif