#include "devtree.h"

void* DEVTREE_BEGIN = 0;
void* DEVTREE_END = 0;

void fdt_traverse( void (*callback)(char *, char *, struct fdt_prop *) ){
    // asm volatile("mov %0, x27;" :  "=r"(DEVTREE_BEGIN));  // QEMU version
    asm volatile("mov %0, x20;" :  "=r"(DEVTREE_BEGIN));  

    // print_hex((uint32_t)DEVTREE_BEGIN);
    char magic[4] = {0xd0, 0x0d, 0xfe, 0xed};

    if (!strncmp((char*)DEVTREE_BEGIN, magic, 4)){
        print_newline();
        print_hex(*(uint32_t*)DEVTREE_BEGIN);
        print_str("\nMagic Fail");
        return;
    }

    struct fdt_header* devtree_header = DEVTREE_BEGIN;
    DEVTREE_END = DEVTREE_BEGIN + devtree_header->totalsize;

    void *dt_struct_addr = DEVTREE_BEGIN + to_little_endian(devtree_header->off_dt_struct);
    char *dt_string_addr = DEVTREE_BEGIN + to_little_endian(devtree_header->off_dt_strings);

    char *node_name;
    char *prop_name;
    uint32_t token;
    uint32_t offset;

    while (1){
        token = to_little_endian(*((uint32_t*)dt_struct_addr));

        if (token == FDT_BEGIN_NODE){

            node_name = dt_struct_addr + 4;
            offset = 4 + strlen(node_name) + 1;

            if (offset%4)
                offset = 4 * ((offset+4) / 4);

            dt_struct_addr += offset;

        }else if (token == FDT_END_NODE){
            dt_struct_addr += 4;
        }else if (token == FDT_PROP){

            struct fdt_prop* prop = (struct fdt_prop*)(dt_struct_addr+4);
            offset = 4 + 8 + to_little_endian(prop->len);

            if (offset%4)
                offset = 4 * ((offset+4) / 4);

            dt_struct_addr += offset;

            prop_name = dt_string_addr + to_little_endian(prop->nameoff);
            callback(node_name, prop_name, prop);

        }else if (token == FDT_NOP){
            dt_struct_addr += 4;
        }else if (token == FDT_END){
            dt_struct_addr += 4;
            break;
        }else {
            print_str("\nToken not matched");
        }
    }
}