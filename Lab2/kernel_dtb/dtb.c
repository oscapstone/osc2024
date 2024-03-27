#include "dtb.h"
#include "uart.h"
#include "helper.h"
#include "shell.h"


struct fdt_header {
    unsigned int magic;
    unsigned int totalsize;
    unsigned int off_dt_struct;
    unsigned int off_dt_strings;
    unsigned int off_mem_rsvmap;
    unsigned int version;
    unsigned int last_comp_version;
    unsigned int boot_cpuid_phys;
    unsigned int size_dt_strings;
    unsigned int size_dt_struct;
};


unsigned int big_to_little_endian(unsigned int value) {
    return ((value & 0xFF000000) >> 24) | \
           ((value & 0x00FF0000) >> 8) | \
           ((value & 0x0000FF00) << 8) | \
           ((value & 0x000000FF) << 24);
}


void parse_new_node(char *address, char *string_address, char *target, void (*callback)(char *))
{
    while (*(address) == DT_BEGIN_NODE_TOKEN)
    {
        // skip: node name

        while (*(address++) != NULL)
            ;
        while (*(address++) == NULL)
            ;
        address--;

        // properties
        while (*address == DT_PROP_TOKEN)
        {
            address++;

            int len = get_int(address);
            address += 4;

            // key
            int temp = get_int(address);
            address += 4;

            if (string_compare(string_address + temp, target))
            {
                callback((char *)get_int(address));
            }

            // skip: value

            address += len;
            while (*(address++) == NULL)
                ;
            address--;
        }

        // children
        parse_new_node(address, string_address, target, callback);
    }

    while (*(address++) != DT_END_NODE_TOKEN)
        ;

    while (*(address++) == NULL)
        ;
    address--;
}

char dt_check_magic_number(char *address)
{
    unsigned int magic_number = get_int(address);

    return DT_MAGIC_NUMBER == magic_number;
}

void dt_tranverse(char *address, char *target_property, void (*callback)(char *))
{
    struct fdt_header * header = (struct fdt_header *) address;
    unsigned int temp;
    unsigned int offset_struct, offset_strings;

    if (!dt_check_magic_number(address))
    {
        uart_puts("Invalid device tree\n");
        return;
    }

    offset_struct = big_to_little_endian(header -> off_dt_struct); //value in dtb is big endian
    offset_strings = big_to_little_endian(header -> off_dt_strings);

    char *newAddress = address + offset_struct;
    
    // point to the dtb structure
    while (*newAddress == NULL) {
        newAddress++;
    }

    // nodes
    while (*(newAddress) != DT_END_TOKEN)
    {
        parse_new_node(newAddress, address + offset_strings, target_property, callback);

        while (*newAddress != DT_END_NODE_TOKEN) {
            newAddress++;
        }
        newAddress++;

        while (*newAddress == NULL) {
            newAddress++;
        }
    }
}