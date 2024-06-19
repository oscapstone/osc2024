#include "fdt.h"
#include "mini_uart.h"
#include "c_utils.h"

/*
Structure Block: The structure block contains a linearized tree of device nodes and properties. 
Each node in the tree represents a device or a logical part of a device, and properties are attributes 
of these devices. The structure block consists of a sequence of tokens that represent the structure and 
content of the device tree. For example, there are tokens for the start and end of a node, property definitions, 
and more. The structure block doesn't contain the actual property names; instead, 
it contains offsets into the strings block where the corresponding property names are stored.

Strings Block: The strings block contains a list of null-terminated strings, which are the property 
names referred to in the structure block. When a property is defined in the structure block, 
it includes an offset that points to the location of the property's name in the strings block.
*/

void fdt_structure_parser(dtb_callback callback, char *cur_struct_ptr, char *addr_dt_strings, unsigned int size_dt_struct)
{
    char *struct_end = cur_struct_ptr + size_dt_struct;
    unsigned int len;
    char *nameoff;

    while (cur_struct_ptr < struct_end)
    {
        unsigned int token = endian_big2little(*(unsigned int *)cur_struct_ptr);
        cur_struct_ptr += 4;

        switch (token)
        {
        case FDT_BEGIN_NODE:
            len = strlen(cur_struct_ptr) + 1;
            if (len %4 != 0)
                len = (len + 4 - len % 4);
            cur_struct_ptr += len;                      // align 32 bits padding
            break;

        case FDT_END_NODE:
            break;
        
        case FDT_PROP:
            // len | name offset | property’s value
            len = endian_big2little(*(unsigned int *)cur_struct_ptr);
            cur_struct_ptr += 4;

            nameoff = (char *)addr_dt_strings + endian_big2little(*(unsigned int *)cur_struct_ptr);
            cur_struct_ptr += 4;

            callback(nameoff, cur_struct_ptr);          // property’s value, byte string of length len(address)
            if (len %4 != 0)
                len = (len + 4 - len % 4);
            cur_struct_ptr += len;              
            break;

        case FDT_NOP:
            break;

        case FDT_END:
            break;

        default:
            return;
        }
    }
}

void fdt_traverse(dtb_callback callback, void *dtb_addr)
{
    fdt_t *header = (fdt_t *)(dtb_addr);
    if (endian_big2little(header->magic) != 0xD00DFEED)
    {
        uart_send_string("Error: fdt_traverse: wrong magic number...\n");
        return;
    }
 
    unsigned int size_dt_struct = endian_big2little(header->size_dt_struct);

    char *addr_dt_struct = (char *)header + endian_big2little(header->off_dt_struct);     // startpoint of structure block
    char *addr_dt_strings = (char *)header + endian_big2little(header->off_dt_strings);   // startpoint of strings block

    fdt_structure_parser(callback, addr_dt_struct, addr_dt_strings, size_dt_struct);

}
