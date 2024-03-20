#include "devtree.h"
#include "string.h"
#include "uart.h"
#include "utils.h"

// Assign a non-zero value to be stored in the .data section
void *DTB_BASE = (void *)0xF;

/**
 * @brief Convert a 4-byte big-endian sequence to little-endian.
 * 
 * @param s: big-endian sequence
 * @return little-endian sequence
 */
static uint32_t be2le(const void *s)
{
    const uint8_t *bytes = (const uint8_t *)s;
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}

void fdt_traverse(void (*callback)(void *))
{
    uart_puts("[INFO] Dtb is loaded at ");
    uart_hex((uintptr_t)DTB_BASE);
    uart_putc('\n');

    uintptr_t dtb_ptr = (uintptr_t)DTB_BASE;
    struct fdt_header *header = (struct fdt_header *)dtb_ptr;

    // Check the magic number
    if (be2le(&(header->magic)) != 0xD00DFEED)
        uart_puts("[ERROR] Dtb header magic does not match!\n");

    uintptr_t structure = (uintptr_t)header + be2le(&header->off_dt_struct);
    uintptr_t strings = (uintptr_t)header + be2le(&header->off_dt_strings);
    uint32_t totalsize = be2le(&header->totalsize);

    // Parse the structure block
    uintptr_t ptr = structure; // Point to the beginning of structure block
    while (ptr < strings + totalsize) {
        uint32_t token = be2le((char *)ptr);
        ptr += 4; // Token takes 4 bytes

        switch (token) {
        case FDT_BEGIN_NODE:
            ptr += align4(strlen((char *)ptr));
            break;
        case FDT_END_NODE:
            break;
        case FDT_PROP:
            uint32_t len = be2le((char *)ptr);
            ptr += 4;
            uint32_t nameoff = be2le((char *)ptr);
            ptr += 4;
            if (!strcmp((char *)(strings + nameoff), "linux,initrd-start")) {
                callback((void *)(uintptr_t)be2le((void *)ptr));
            }
            ptr += align4(len);
            break;
        case FDT_NOP:
            break;
        case FDT_END:
            break;
        }
    }
}