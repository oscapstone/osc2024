#include <bsp/uart.h>
#include <kernel/device_tree.h>
#include <kernel/io.h>
#include <lib/string.h>
#include <lib/utils.h>

// get this value from start.S
extern void *dtb_base;

static uint64_t initrd_start, initrd_end;
static uint32_t be2le(const void *s);

void fdt_traverse(void (*callback)(void *)) {
    struct fdt_header *header = (struct fdt_header *)(dtb_base);
    print_string("[INFO] Dtb loaded address: ");
    uart_hex((uintptr_t)header);
    print_string("\n");

    // get the offset of the structure block and string block by adding the base
    // address of the header to the offset
    uint8_t *structure = (uint8_t *)header + be2le(&header->off_dt_struct);
    uint8_t *strings = (uint8_t *)header + be2le(&header->off_dt_strings);

    uint32_t totalsize = be2le(&header->totalsize);

    // Parse the structure block
    uint8_t *ptr = structure;  // Point to the beginning of structure block
    while (ptr < strings + totalsize) {
        uint32_t token = be2le((char *)ptr);
        ptr += 4;  // Token takes 4 bytes

        switch (token) {
            case FDT_BEGIN_NODE:
                // print_string((char *)ptr);
                ptr += align4(strlen((char *)ptr));
                break;
            case FDT_END_NODE:
                break;
            case FDT_PROP:;  // for C23 warning
                uint32_t len = be2le((char *)ptr);
                ptr += 4;
                uint32_t nameoff = be2le((char *)ptr);
                ptr += 4;
                if (strcmp((char *)(strings + nameoff), "linux,initrd-start") ==
                    0) {
                    initrd_start = be2le((void *)ptr);
                    callback((void *)initrd_start);
                } else if (strcmp((char *)(strings + nameoff),
                                  "linux,initrd-end") == 0) {
                    initrd_end = be2le((void *)ptr);
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

uint64_t fdt_get_initrd_start() { return initrd_start; }
uint64_t fdt_get_initrd_end() { return initrd_end; }

/**
 * @brief Convert a 4-byte big-endian sequence to little-endian.
 *
 * @param s: big-endian sequence
 * @return little-endian sequence
 */
static uint32_t be2le(const void *s) {
    const uint8_t *bytes = (const uint8_t *)s;
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}
