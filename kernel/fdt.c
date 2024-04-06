#include "fdt.h"
#include "malloc.h"
#include "string.h"
#include "uart.h"

// Linux fdt.c
//      https://elixir.free-electrons.com/linux/v4.15-rc9/source/drivers/of/fdt.c#L445
// unflatten tree
//      https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/flattree.c#L745
static fdt_header_t *fdt_header;
static int initramfs_addr;
void fdt_traverse(fdt_callback_t cb)
{
    // parse header
    asm volatile("mov %0, x28" : "=r"(fdt_header));

    // swap little to big endian
    uint32_t *p = (uint32_t *) fdt_header;
    for (int i = 0; i < 10; i++) {
        p[i] = swap_endian(p[i]);
    }

    // create blob object to device tree binary
    blob_t *blob = (blob_t *) malloc(sizeof(blob_t));
    blob->base = (uint32_t *) fdt_header; 
    blob->ptr = blob->base;
    BLOB_ADVANCE(blob, fdt_header->off_dt_struct);

    // parse fdt
    unflatten_tree(blob);

    // execute callback fn
    cb(initramfs_addr);
}

void unflatten_tree(blob_t *blob)
{
    do {
        switch (swap_endian(*blob->ptr)) {
        case FDT_BEGIN_NODE:
            BLOB_ADVANCE(blob, sizeof(uint32_t));
            BLOB_ADVANCE(blob, PADDING_4(strlen((const char *) blob->ptr) + 1));
            break;

        case FDT_END_NODE:
            BLOB_ADVANCE(blob, sizeof(uint32_t));
            break;

        case FDT_PROP:
            BLOB_ADVANCE(blob, sizeof(uint32_t));

            fdt_propmeta_t *meta = (fdt_propmeta_t *) blob->ptr;
            BLOB_ADVANCE(blob, sizeof(fdt_propmeta_t));

            char *propname = (char *) blob->base + fdt_header->off_dt_strings + swap_endian(meta->nameoff);
            if (!strcmp("linux,initrd-start", propname)) {
                initramfs_addr = swap_endian(*((uint32_t *) blob->ptr));
                return;
            }

            BLOB_ADVANCE(blob, PADDING_4(swap_endian(meta->len)));
            break;

        case FDT_END:
            break;

        case FDT_NOP:
            BLOB_ADVANCE(blob, sizeof(uint32_t));
            break;

        case 0: // what?
            BLOB_ADVANCE(blob, sizeof(uint32_t));
            break;

        default:
            break;
        }
    } while (1);

    return;
}
