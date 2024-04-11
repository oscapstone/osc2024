#include "dtb.h"
#include "string.h"
#include "uart.h"

extern char *cpio_start;

unsigned int big2little_endian(unsigned int big)
{
    // arm is little endian
    return ((big & 0x000000FF) << 24) | ((big & 0x0000FF00) << 8) | ((big & 0x00FF0000) >> 8) |
           ((big & 0xFF000000) >> 24);
}

void fdt_traverse(dtb_callback callback, char *dtb_base)
{
    struct fdt_header *header = (struct fdt_header *)dtb_base;

    if (big2little_endian(header->magic) != FDT_MAGIC) {
        uart_puts("Invalid FDT header\n");
        return;
    }

    unsigned int struct_size = big2little_endian(header->size_dt_struct);
    char *dt_struct_ptr = (char *)((char *)header + big2little_endian(header->off_dt_struct));
    char *dt_strings_ptr = (char *)((char *)header + big2little_endian(header->off_dt_strings));
    char *end = (char *)dt_struct_ptr + struct_size;
    char *start = dt_struct_ptr;

    while (start < end) {
        unsigned int token_type = big2little_endian(*(unsigned int *)start);
        start += 4;

        switch (token_type) {
        case FDT_BEGIN_NODE:
            start += strlen(start);
            start += (4 - ((unsigned long long)start % 4));
            break;
        case FDT_PROP:
            unsigned int len = big2little_endian(*(unsigned int *)start);
            start += 4;

            char *name = (char *)dt_strings_ptr + big2little_endian(*(unsigned int *)start);
            start += 4;

            callback(token_type, name, start, len);
            start += len;
            if ((unsigned long long)start % 4 != 0)
                start += 4 - (unsigned long long)start % 4;
            break;
        case FDT_END_NODE:
        case FDT_NOP:
        case FDT_END:
            break;
        default:
            return;
        }
    }
}

void initramfs_callback(unsigned int node_type, char *name, void *value, unsigned int name_size)
{
    if (!strcmp(name, "linux,initrd-start"))
        cpio_start = (char *)(unsigned long long)big2little_endian(*(unsigned int *)value);
}
