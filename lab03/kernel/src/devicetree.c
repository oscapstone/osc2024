#include "devicetree.h"
#include "string.h"
#include "io.h"

void fdt_traverse(void(*callback_func)(char*, char*, fdt_prop*))
{
    fdt_header *dt_header = (fdt_header*)(*DTB_ADDR);
    printf("\nDTB Address (Function): ");
    printf_hex(dt_header);

    uint32_t dt_struct_off = endian_swap(dt_header->off_dt_struct);
    uint32_t dt_string_off = endian_swap(dt_header->off_dt_strings);
    
    void* dt_struct_addr = *DTB_ADDR + dt_struct_off;
    char* dt_string_addr = *DTB_ADDR + dt_string_off;


    printf("\nDT String Address: ");
    printf_hex(dt_string_addr);
    printf("\nDT Struct Address: ");
    printf_hex(dt_struct_addr);

    printf("\nMagic: ");
    printf_hex(endian_swap(dt_header->magic));
    if(dt_header->magic == (uint32_t)0xd00dfeed)
    {
        printf("Magic Failed\n");
        return;
    }


    char* node_name;
    char* prop_name;

    while(1)
    {
        uint32_t token = endian_swap((*(uint32_t*)dt_struct_addr));
        
        switch (token)
        {
            case FDT_BEGIN_NODE: // It shall be followed by the node’s unit name as extra data.
            {
                node_name = (char*)dt_struct_addr + 4;  // tage size is 32bits = 4bytes
                uint32_t size = strlen(node_name);      // size of node name
                uint32_t offset = 4 + size + 1;

                if(offset % 4 !=0) offset = ((offset/4 + 1)*4); // align to 4bytes (32bits)
                dt_struct_addr = dt_struct_addr + offset;
                
                break;
            }
            case FDT_END_NODE: //This token has no extra data; so it is followed immediately by the next token
            {
                dt_struct_addr = dt_struct_addr + 4; break;
            }
            case FDT_PROP:
            {
                fdt_prop* prop_ptr = (fdt_prop*)(dt_struct_addr + 4);
                uint32_t offset = 4 + sizeof(fdt_prop) + endian_swap(prop_ptr->len);

                if(offset % 4 != 0) offset = ((offset/4 + 1)*4);
                dt_struct_addr = dt_struct_addr + offset;

                prop_name = (void*)dt_string_addr + endian_swap(prop_ptr->nameoff);

                callback_func(node_name, prop_name, prop_ptr);

                break;
            }
            case FDT_NOP:
            {
                dt_struct_addr = dt_struct_addr + 4; break;
            }
            case FDT_END:
            {
                dt_struct_addr = dt_struct_addr + 4; break;
            }
            default:
                return;
        }
    }

}

// big endian to little endian
uint32_t endian_swap(uint32_t value)
{
    uint32_t ret = 0;
    uint8_t* ptr = (void*)&value;
    ret = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    return ret;
}

