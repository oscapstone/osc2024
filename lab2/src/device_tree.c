#include "device_tree.h"
#include "mini_uart.h"

char* DTB_ADDR;
void* struct_ptr, * string_ptr;

int strlen(char* str) {
    int len = 0;
    while(*(str+len) != '\0') { len++; }
    return len + 1;
}

int dt_strcmp(char* a, char* b, int len) {
    for(int i = 0; i < len; i++) {
        if(*(a+i) != *(b+i)) { return 0; }
    }
    return 1;
}

unsigned int swap_endian(unsigned int b_end) { // uint32_t
    unsigned int l_end; 
    unsigned int b0, b1, b2, b3;
    b0 = (b_end & 0x000000ff) << 24;
    b1 = (b_end & 0x0000ff00) << 8;
    b2 = (b_end & 0x00ff0000) >> 8;
    b3 = (b_end & 0xff000000) >> 24;
    l_end = b0 | b1 | b2 | b3;
    return l_end;
}

void get_dtb_addr() {
    asm volatile("MOV %[addr], x25" : [addr] "=r" (DTB_ADDR));
    struct fdt_header *header = (struct fdt_header*) DTB_ADDR;
    if(swap_endian(header->magic) == FDT_HEADER_MAGIC) { uart_send_string("magic same check\r\n"); }

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
            name_len = strlen(node_name);

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

