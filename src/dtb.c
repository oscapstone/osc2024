#include "dtb.h"
#include "uart.h"
#include "file.h"
#include "stdint.h"
#include "string.h"
#include "initrd.h"
#include "stddef.h"
#include "memblock.h"
#include "kernel.h"

extern uint64_t __dtb_address;

fdt_header *dtb_address;

void fdt_init()
{
    uint64_t *tmp_pointer = (uint64_t *) &__dtb_address;
    dtb_address = (fdt_header *) *tmp_pointer;

    fdt_traverse(initramfs_callback);
    printf("==== init: DTB & INITRD\n");
}

void fdt_reserve_memory(void)
{
    int len = ALIGN(bswap_32(dtb_address->totalsize), 4096);
    char *buf = (char *) dtb_address;

    if (bswap_32(dtb_address->magic) != FDT_MAGIC)
        return;

    memblock_reserve((unsigned long) buf, ALIGN(len, 8));
}

/* Start from dtb_address, traverse all the node and property. */
void fdt_traverse(void (*callback)(fdt_prop *, char *, char *))
{
    if (bswap_32(dtb_address->magic) != FDT_MAGIC)
        return;

    uint32_t *struct_sp = (uint32_t *) ((char *)dtb_address + bswap_32(dtb_address->off_dt_struct)); // offset to structure
    char *string_sp = (char *) ((char *)dtb_address + bswap_32(dtb_address->off_dt_strings)); // offset to string
    
    char *node_name = NULL;
    bool END = false;

    while (!END) {
        uint32_t token = bswap_32(*struct_sp);

        switch (token) {
        case FDT_BEGIN_NODE:;
            fdt_node_header *node = (fdt_node_header *) struct_sp;
            node_name = node->name;
            struct_sp += ALIGN(strlen(node->name), 4) / 4;
            break;
        case FDT_PROP:;
            fdt_prop *prop = (fdt_prop*)(struct_sp + 1); // skip the token (flag)
            struct_sp += (sizeof(fdt_prop) + ALIGN(bswap_32(prop->len), 4)) / 4;
            char *property_name = string_sp + bswap_32(prop->nameoff);
            callback(prop, node_name, property_name);
            break;
        case FDT_END:
            END = true;
            break;
        default:
            break;
        }

        struct_sp++;
    }
}