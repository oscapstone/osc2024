#include "../include/dtb.h"


char *cpio_addr;
char *dtb_addr;

/*bcm2710-rpi-3-b-plus.dtb is a binary representation of the device tree for the Raspberry Pi 3 Model B Plus. 
It contains the hardware description necessary for the operating system to understand the hardware layout and resources.
*/


/* structur of DTB format (page 53)
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


// FDT uses big-endian format, while the ARM CPU in Raspberry Pi uses little-endian
// swaps the endianness of a 32-bit integer from big-endian to little-endian.
uint32_t fdt32_to_cpu(uint32_t x) {
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
}


void fdt_traverse(void* fdt, fdt_callback_t callback) {
    struct fdt_header* header = (struct fdt_header*)fdt;

    // Validate the magic number
    if (fdt32_to_cpu(header->magic) != 0xd00dfeed) {
        uart_puts("Invalid FDT magic number\n");
        return;
    }

    uint32_t off_dt_struct = fdt32_to_cpu(header->off_dt_struct);
    uint32_t off_dt_strings = fdt32_to_cpu(header->off_dt_strings);

    // Pointers to the structure and strings blocks
    uint8_t* struct_block = (uint8_t*)fdt + off_dt_struct;
    char* strings_block = (char*)fdt + off_dt_strings;

    uint8_t* p = struct_block;
    while (p < struct_block + fdt32_to_cpu(header->size_dt_struct)) {
        /* The structure block is composed of a sequence of pieces, each beginning with a token (a big-endian 32-bit integer).
        Some tokens are followed by extra data, the format of which is determined by the token value. (pg. 56)
        */
        
        uint32_t token = fdt32_to_cpu(*(uint32_t*)p);
        p += 4;

        //The FDT_BEGIN_NODE token marks the beginning of a node’s representation.
        // followed by the node’s unit name as extra data
        if (token == FDT_BEGIN_NODE) {
            const char* name = (const char*)p;
            p += strlen(name) + 1;

            // All tokens in Structure Block shall be aligned on a 32-bit boundary (pg. 56)
            p = (uint8_t*)(((uintptr_t)p + 3) & ~3);

            // Traverse properties
            // The FDT_PROP token marks the beginning of the representation of one property in the devicetree.
            /*
            struct {
                uint32_t len;                  //len gives the length of the property’s value in bytes
                uint32_t nameoff;              //nameoff gives an offset into the strings block
            }
            */
            while (fdt32_to_cpu(*(uint32_t*)p) == FDT_PROP) {
                p += 4;
                uint32_t len = fdt32_to_cpu(*(uint32_t*)p);
                p += 4;
                uint32_t nameoff = fdt32_to_cpu(*(uint32_t*)p);
                p += 4;

                const char* prop_name = strings_block + nameoff;

                //After the structure, the property’s value is given as a byte string of length len. 
                //This value is followed by zeroed padding bytes (if necessary) to align to the next 32-bit boundary
                const void* value = (const void*)p;

                // prevent execute a NLLL function
                if (callback) {
                    callback(name, prop_name, value, len);
                }

                p += len;
                // Align to 4 bytes
                p = (uint8_t*)(((uintptr_t)p + 3) & ~3);
            }
        //The FDT_END_NODE token marks the end of a node’s representation. 
        } else if (token == FDT_END_NODE) {
            // Do nothing, just move to the next token

        //The FDT_NOP token will be ignored by any program parsing the device tree.
        } else if (token == FDT_NOP) {
            // Do nothing, just move to the next token
        
        //The FDT_END token marks the end of the structure block.
        } else if (token == FDT_END) {
            break;
        } else {
            uart_puts("Unknown token\n");
            break;
        }
    }
}



void initramfs_callback(const char* name, const char* prop, const void* value, int len) {
    if (strcmp(prop, "linux,initrd-start") == 0) {
        cpio_addr = (char *)(uintptr_t)fdt32_to_cpu(*(uint32_t*)value);
        uart_puts("Initramfs start address: 0x");
        uart_hex(cpio_addr);
        uart_puts("\n");
    }
}


