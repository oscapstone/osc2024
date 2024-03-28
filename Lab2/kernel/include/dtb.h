#ifndef _DTB_H_
#define _DTB_H_
#include "uart.h"

#define PADDING         0x00000000
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

#define DT_ADDR         0x200000

typedef void (*dtb_callback)(UI node_type, char *name, void *value, UI name_size);

uint32_t uint32_endian_big2lttle(uint32_t data);
void traverse_device_tree(dtb_callback callback); // traverse dtb tree
void dtb_callback_show_tree(uint32_t node_type, char *name, void *value, uint32_t name_size);
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size);

/* All the header fields are 32-bit integers, stored in big-endian format. */
typedef struct _fdt_header
{
    UI magic;             /* The value 0xd00dfeed (big-endian).*/
    UI totalsize;         /* The total size in bytes of the devicetree data structure. */
    UI off_dt_struct;     /* The offset in bytes of the structure block from the beginning of the header.*/
    UI off_dt_strings;    /* The offset in bytes of the strings block from the beginning of the header.*/
    UI off_mem_rsvmap;    /* The offset in bytes of the memory reservation block from the beginning of the header.*/
    UI version;           /* The version of the devicetree data structure.*/
    UI last_comp_version; /* The lowest version of the devicetree data structure with which the version used is backwards compatible*/
    UI boot_cpuid_phys;   /* The physical ID of the system’s boot CPU.*/
    UI size_dt_strings;   /* The length in bytes of the strings block section of the devicetree blob.*/
    UI size_dt_struct;    /* The length in bytes of the structure block section of the devicetree blob.*/
} fdt_header;

typedef struct _fdt_reserve_entry
{ 
    UL address; 
    UL size;
} fdt_reserve_entry;

typedef struct _fdt_property
{
    uint32_t len;
    uint32_t nameoff;
} fdt_property;

#endif
