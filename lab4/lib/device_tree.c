#include "device_tree.h"
#include "mini_uart.h"
#include "str_util.h"

char* DTB_ADDR;
void* struct_ptr, * string_ptr;

void get_dtb_addr() {
    asm volatile("MOV %[addr], x25" : [addr] "=r" (DTB_ADDR));
    struct fdt_header *header = (struct fdt_header*) DTB_ADDR;
    if(swap_endian(header->magic) == FDT_HEADER_MAGIC) { uart_send_string("magic same check\r\n"); }
    uart_send_string("DTB_ADDR = 0x");
    uart_send_hex((unsigned long)DTB_ADDR);
    uart_send_string("\r\n");
    struct_ptr = DTB_ADDR + swap_endian(header->off_dt_struct);
    if(swap_endian(*((unsigned int *)struct_ptr)) == FDT_BEGIN_NODE) { uart_send_string("first node same check\r\n"); }

    string_ptr = DTB_ADDR + swap_endian(header->off_dt_strings);
}

void dev_tree_parser( void (*callback)(char *node_name, char *prop_name, struct fdt_lex_prop *prop) ) {
    get_dtb_addr();

    unsigned int token_type;
    unsigned int offset;
    char *node_name, *prop_name;
    int name_len;
    struct fdt_lex_prop *prop;
    

    while(1) {
        token_type = swap_endian(*((unsigned int*)struct_ptr));
        if(token_type == FDT_BEGIN_NODE) {
            node_name = struct_ptr + 4;
            name_len = str_len(node_name);

            offset = 4 + name_len;
            if(offset % 4 > 0) { offset += ( 4 - (offset % 4) ); }// padding to 32 bit
            struct_ptr += offset;
        }
        else if(token_type == FDT_PROP) {
            prop = (struct fdt_lex_prop*) (struct_ptr + 4);
            prop_name = string_ptr + swap_endian(prop->nameoff);

            offset = 4 + 8 + swap_endian(prop->len);
            if(offset % 4 > 0) { offset += ( 4 - (offset % 4) ); }
            struct_ptr += offset;

            callback(node_name, prop_name, prop);
        }
        else if(token_type == FDT_NOP || token_type == FDT_END_NODE) {
            struct_ptr += 4;
        }
        else if(token_type == FDT_END) {
            break;
        }
        else {
            uart_send_string("broke: token mismatch\r\n");
            break;
        }
    }
}

