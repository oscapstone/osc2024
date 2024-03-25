#ifndef __DEVICETREE_H__
#define __DEVICETREE_H__

#include "type.h"
#include "string.h"
#include "io.h"

/*
0x50000: devicetree blob address
*/

#define DTB_ADDR ((volatile uint64_t *)0x50000)

#define FDT_BEGIN_NODE  0x01
#define FDT_END_NODE    0x02
#define FDT_PROP        0x03
#define FDT_NOP         0x04
#define FDT_END         0x09

/*
    |-------------------|
    | struct fdt_header |
    |-------------------|
    | free space        |
    |-------------------|
    | memory reservation| -> list of areas in physical memory which are reserved
    |-------------------|
    | free space        |
    |-------------------|
    | structure block   |
    |-------------------|
    | free space        |
    |-------------------|
    | strings block     |
    |-------------------|
    | free space        |
    |-------------------|
*/

/* All the header fields are 32-bit integers, stored in big-endian format. */
typedef struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;         // total size of DT block
    uint32_t off_dt_struct;     // offset to the structure block 
    uint32_t off_dt_strings;    // offset to the strings block
    uint32_t off_mem_rsvmap;    // offset to the memory reservation block
    uint32_t version;
    uint32_t last_comp_version; 
    uint32_t boot_cpuid_phys;   // physical address of the boot CPU
    uint32_t size_dt_strings;   // size of the strings block
    uint32_t size_dt_struct;    // size of the structure block
} fdt_header;


/*
    Memory reservation block
    Composed of sequence of tokens with data shown below.
    Each token in the structure block, and thus the structure block itself, 
    shall be loacted at a 4-byte aligned offset from the beginning of the devicetree blob.

    Five tokens types: (32-bit big-endian integers)
    1. FDT_BEGIN_NODE   (0x00000001)    be followed by the node's uint name as extra data
    2. FDT_END_NODE     (0x00000002)  
    3. FDT_PROP         (0x00000003)
    4. FDT_NOP          (0x00000004)
    5. FDT_END          (0x00000009)

    FDT_BEGIN_NODE -> ... -> FDT_END_NODE

    - (Optional) Any number of FDT_NOP tokens
    - FDT_BEGIN_NODE
        - The node’s name as a null-terminated string
        - [zeroed padding bytes to align to a 4-byte boundary]
    - For each property of the node
        - (Optional) Any number of FDT_NOP tokens
        - FDT_PROP token
            * property infomation
            * aligned to a 4-byte boundary
*/

/*
    The strings block contains strings representing all the property names used in the tree.
*/


/* 32-bit big-endian integers */
typedef struct fdt_prop {
    uint32_t len;       // length of the property value
    uint32_t nameoff;   // offset into the strings block for the property name
} fdt_prop;

void fdt_traverse(void(*callback_func)(char*, char*, fdt_prop*));
uint32_t endian_swap(uint32_t value);

#endif