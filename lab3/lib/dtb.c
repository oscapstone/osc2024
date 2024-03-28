#include "dtb.h"
// #include "uart.h"
char* DTB_ADDRESS;
void* INITRD_ADDR;

void dtb_init(uint64_t x0) {
    DTB_ADDRESS = (char*)x0;
    // uart_putc(*DTB_ADDRESS + 0x60, 160);
    dtb_parser(dtb_get_initrd_callback);
}

void dtb_parser(dtb_callback_t callback) {
    fdt_header* header = (fdt_header*)DTB_ADDRESS;

    // Check magic
    if (get_be_uint32(&header->magic) != 0xd00dfeed) {
        uart_write_string("[+] BAD\r\n");
        return;
    }

    char *dt_sturct = (char*)header + get_be_uint32(&header->off_dt_struct),
         *dt_strings = (char*)header + get_be_uint32(&header->off_dt_strings);
    char* ptr = dt_sturct;
    uint32_t token_type;
    uint32_t len;

    while (1) {
        token_type = get_be_uint32(ptr);
        ptr += 4;

        switch (token_type) {
            case FDT_BEGIN_NODE:
                callback(token_type, ptr, 0);
                ptr += strlen(ptr) + 1;
                if ((uint64_t)ptr % 4) ptr += 4 - (uint64_t)ptr % 4;  // 4 bytes alignment
                break;
            case FDT_END_NODE:
                callback(token_type, 0, 0);
                break;
            case FDT_PROP:
                len = get_be_uint32(ptr);
                ptr += 4;
                char* name = (char*)dt_strings + get_be_uint32(ptr);  // dt_strings + nameoff
                ptr += 4;
                callback(token_type, name, ptr);
                ptr += len;
                if ((uint64_t)ptr % 4) ptr += 4 - (uint64_t)ptr % 4;  // 4 bytes alignment
                break;
            case FDT_NOP:
                callback(token_type, 0, 0);
                break;
            case FDT_END:
                callback(token_type, 0, 0);
                break;
            default:
                uart_puth(*ptr);
                return;
                break;
        }
        if (token_type == FDT_END) break;
    }
}

void dtb_get_initrd_callback(uint32_t token_type, char* name, char* data) {
    if (token_type == FDT_PROP && !strcmp(name, "linux,initrd-start")) {
        INITRD_ADDR = get_be_uint32(data);

        uart_write_string("[+] Initrd address: 0x");
        uart_puth(data);
        uart_write_string("\r\n");
    }
}

void dtb_show_callback(uint32_t token_type, char* name, char* data) {
    static unsigned int level = 0;
    switch (token_type) {
        case FDT_BEGIN_NODE:
            for (uint32_t i = 0; i < level; i++) uart_write_string("    ");
            uart_write_string(name);
            uart_write_string(" {\r\n");
            level++;
            break;

        case FDT_END_NODE:
            level--;
            for (uint32_t i = 0; i < level; i++) uart_write_string("    ");
            uart_write_string("}\r\n");
            break;

        case FDT_PROP:
            for (uint32_t i = 0; i < level; i++) uart_write_string("    ");
            uart_write_string(name);
            uart_write_string("\r\n");
            break;
    }
}