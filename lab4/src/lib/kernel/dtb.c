#include "dtb.h"
#include "memory.h"
#include "string.h"
#include "uart.h"

extern char *cpio_start;
extern char *cpio_end;

static char *dtb_ptr;

struct fdt_reserve_entry {
    unsigned long long address;
    unsigned long long size;
};

unsigned int big2little_endian(unsigned int big)
{
    // arm is little endian
    return ((big & 0x000000FF) << 24) | ((big & 0x0000FF00) << 8) | ((big & 0x00FF0000) >> 8) |
           ((big & 0xFF000000) >> 24);
}

void fdt_traverse(dtb_callback callback, char *dtb_base)
{
    dtb_ptr = dtb_base;
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

    if (!strcmp(name, "linux,initrd-end"))
        cpio_end = (char *)(unsigned long long)big2little_endian(*(unsigned int *)value);
}

void find_reserved_memory()
{
    struct fdt_header *header = (struct fdt_header *)dtb_ptr;

    if (big2little_endian(header->magic) != FDT_MAGIC) {
        uart_puts("Invalid FDT header\n");
        return;
    }

    char *dt_mem_rsvmap = (char *)((char *)header + big2little_endian(header->off_mem_rsvmap));
    struct fdt_reserve_entry *entry = (struct fdt_reserve_entry *)dt_mem_rsvmap;

    while (entry->address != 0 || entry->size != 0) {
        unsigned long long start = big2little_endian(entry->address);
        unsigned long long end = start + big2little_endian(entry->size);
        memory_reserve(start, end);
        entry++;
    }

    memory_reserve((unsigned long long)dtb_ptr, (unsigned long long)dtb_ptr + big2little_endian(header->totalsize));
}
