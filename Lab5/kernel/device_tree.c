#include "device_tree.h"
#include "../peripherals/mini_uart.h"
#include "initramdisk.h"

DTBHeader myHeader;

// For listing all the child node names of the reserved memory node.
short rvmem_child_cnt;

// Initialize the header struct.
// return 0 for success, 1 for error.
int fdt_traverse(fdt_node_callback_t callback, uintptr_t dtb_address) {
    uint32_t* dtb = (uint32_t *)dtb_address;

    rvmem_child_cnt = 0;

    // Parse the DTB header fields.
    // __builtin_bswap32() is a GCC built-in function to convert to the system's endianness.(RPi uses little-endian)
    myHeader.magic = __builtin_bswap32(*dtb++);
    if ((myHeader.magic - 0xd00dfeed) != 0) {
        uart_send_string("DTB magic number doesn't match!\r\n");
        return 1;
    }
    myHeader.totalsize = __builtin_bswap32(*dtb++);
    myHeader.off_dt_struct = __builtin_bswap32(*dtb++);
    myHeader.off_dt_strings = __builtin_bswap32(*dtb++);
    myHeader.off_mem_rsvmap = __builtin_bswap32(*dtb++);
    myHeader.version = __builtin_bswap32(*dtb++);
    myHeader.last_comp_version = __builtin_bswap32(*dtb++);
    myHeader.boot_cpuid_phys = __builtin_bswap32(*dtb++);
    myHeader.size_dt_strings = __builtin_bswap32(*dtb++);
    myHeader.size_dt_struct = __builtin_bswap32(*dtb++);

    // uart_send_string("DTB start address: ");
    // uart_send_uint(dtb_address);
    // uart_send_string("\r\n");

    // uart_send_string("DTB end address: ");
    // uart_send_uint(dtb_address + myHeader.totalsize);
    // uart_send_string("\r\n");

    if (parse_struct(callback, dtb_address) != 0) {
        uart_send_string("Error parsing devicetree!\r\n");
    }
}

// Parse the entire structure and call corresponding callback functions to perform
// certain operations.
int parse_struct(fdt_node_callback_t callback, uintptr_t dtb_start_addr) {

    // Point to the beginning of structure block.
    uint32_t* cur_ptr = (uint32_t *)(dtb_start_addr + myHeader.off_dt_struct);

    for (;;) {
        uint32_t token = __builtin_bswap32(*cur_ptr++);

        switch (token) {
            case FDT_BEGIN_NODE:
                // uart_send_string("In FDT_BEGIN_NODE!\r\n");
                // The node name consists of 31 characters at most, with the root node an empty string.
                char name[32];
                // Reset name array.
                for (int i = 0; i < 31; i++) {
                    name[i] = '\0';
                }

                char* name_ptr = (char *)cur_ptr;

                // Root node.
                if (*name_ptr == '\0') {
                    name[0] = '/';
                    name_ptr++;
                // All other nodes has names.
                } else {
                    for (int i = 0; *name_ptr != '\0'; i++, name_ptr++) {
                        name[i] = *name_ptr;
                    }
                    // Ignore the null terminator.
                    name_ptr++;
                }

                cur_ptr = (uint32_t *)ALIGN((uintptr_t)name_ptr, ALIGN_STRUCT_BLOCK);
                
                callback(token, name, NULL, (uintptr_t)cur_ptr);
                break;
            case FDT_END_NODE:
                break;
            case FDT_PROP:
                uint32_t len = __builtin_bswap32(*(uint32_t *)cur_ptr++);
                uint32_t nameoff = __builtin_bswap32(*(uint32_t *)cur_ptr++);

                // Point to the address storing the property name within the strings block.
                char* prop_name_ptr = (char *)(dtb_start_addr + myHeader.off_dt_strings + nameoff);
                char prop_name[32];
                for (int i = 0; i < 32; i++) {
                    prop_name[i] = '\0';
                }

                for (int i = 0; *prop_name_ptr != '\0'; i++, prop_name_ptr++) {
                    prop_name[i] = *prop_name_ptr;
                }

                // Pass the length of the property's value and the pointer to that value
                // into callback function for extracting value for that property.
                callback(token, prop_name, &len, (uintptr_t)cur_ptr);

                // Property's value is given as a byte string of length len(In bytes).
                // cur_ptr is an uint32 pointer, so when it increments by 1, it's
                // actually incrementing 4 bytes.
                if (len % 4 == 0) {
                    cur_ptr += (len / 4);
                } else {
                    cur_ptr += (len / 4 + 1);
                }

                cur_ptr = (uint32_t *)ALIGN((uintptr_t)cur_ptr, ALIGN_STRUCT_BLOCK);

                break;
            case FDT_NOP:
                continue;
                break;
            case FDT_END:
                return 0;
            default:
                break;
        }

    }
}

void print_tree(int type, const char* name, void* data, uintptr_t cur_ptr) {
    switch (type) {
        case FDT_BEGIN_NODE:
            uart_send_string("Node name: ");
            uart_send_string("\r\n");
            uart_send_string((char *)name);
            uart_send_string("\r\n");
            break;

        // Also print the property value.
        case FDT_PROP:
            uart_send_string("Property: ");
            uart_send_string((char *)name);
            uart_send_string("\r\n");
            // Value: <string> or <stringlist>
            if (!strcmp(name, "compatible") || !strcmp(name, "model")) {
                char* val_ptr = (char *)cur_ptr;
                int len = *(int *)data;
                uart_send_string("  ");
                for (int i = 0; i < len; i++) {
                    if (*val_ptr == '\0') {
                        if (i != len - 1) {
                            uart_send_string("; ");
                        } else {
                            uart_send(';');
                        }
                        val_ptr++;

                    } else {
                        uart_send(*val_ptr++);
                    }
                }
                
                uart_send_string("\r\n");
            }
            break;
        default:
            break;
    }
}

void get_initrd_address(int type, const char* name, void* data, uintptr_t cur_ptr) {
    // Address of initramfs is stored within the property name "linux,initrd-start".
    if (type == FDT_PROP) {
        if (strcmp(name, "linux,initrd-start") == 0) {
            uint32_t* val_ptr = (uint32_t *)cur_ptr;
            initramdisk_main(__builtin_bswap32(*val_ptr));
            uart_send_string("initramdisk addr: ");
            uart_send_uint(__builtin_bswap32(*val_ptr));
            uart_send_string("\r\n");
        }
    }
}

void get_initrd_end_address(int type, const char* name, void* data, uintptr_t cur_ptr) {
    // Address of initramfs is stored within the property name "linux,initrd-start".
    if (type == FDT_PROP) {
        if (strcmp(name, "linux,initrd-end") == 0) {
            uint32_t* val_ptr = (uint32_t *)cur_ptr;
            uart_send_string("initramdisk end addr: ");
            uart_send_uint(__builtin_bswap32(*val_ptr));
            uart_send_string("\r\n");
        }
    }
}

int parse_reserved_memory(uintptr_t mem_rsvmap_addr) {
    Mem_Reserve_Entry* entry = (Mem_Reserve_Entry *)mem_rsvmap_addr;

    while (__builtin_bswap64(entry->address) != 0 || __builtin_bswap64(entry->size) != 0) {
        uart_send_string("Memory reserve address: ");
        uart_send_int(__builtin_bswap64(entry->address));
        uart_send_string(" ~ ");
        uart_send_int(__builtin_bswap64(entry->address) + __builtin_bswap64(entry->size));
        uart_send_string("\r\n");
        entry++;
    }

    uart_send_string("End of parsing memory reservation block\r\n");
    return 0;
}

void parse_rvmem_child(int type, const char* name, void* data, uintptr_t cur_ptr) {

    if (type == FDT_BEGIN_NODE) {
        if (strcmp(name, "reserved-memory") == 0) {
            rvmem_child_cnt++;
        } else if (rvmem_child_cnt > 0) {
            uart_send_string("Reserved-memory device: ");
            uart_send_string((char *)name);
            uart_send_string("\r\n");
        }
    } else if (type == FDT_END_NODE) {
        if (rvmem_child_cnt > 0) {
            rvmem_child_cnt--;
        }
    }
}