#include "dtb.h"
#include "uart.h"
#include "helper.h"
#include "shell.h"

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
    unsigned int temp;
    unsigned int offset_struct, offset_strings;

    if (!dt_check_magic_number(address))
    {
        uart_puts("Invalid device tree\n");
        return;
    }

    // skip: total size

    offset_struct = get_int(address + 8);
    offset_strings = get_int(address + 12);

    // skip: off_dt_strings
    // skip: off_mem_rsvmap
    // skip: version
    // skip: last_comp_version
    // skip: boot_cpuid_phys
    // skip: size_dt_strings
    // skip: size_dt_struct

    char *newAddress = address + offset_struct + 1;

    while (*(newAddress++) == NULL)
        ;
    newAddress--;

    // nodes
    while (*(newAddress) != DT_END_TOKEN)
    {
        parse_new_node(newAddress, address + offset_strings, target_property, callback);

        while (*(newAddress++) != DT_END_NODE_TOKEN)
            ;

        while (*(newAddress++) == NULL)
            ;
        newAddress--;
    }
}