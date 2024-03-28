#include "dtb.h"
#include "uart.h"
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

// 16進制轉成2進制，兩個bit變8個binary，就會試value的一個int
unsigned int big_to_little_endian(unsigned int value) {
    return ((value & 0xFF000000) >> 24) | \
           ((value & 0x00FF0000) >> 8) | \
           ((value & 0x0000FF00) << 8) | \
           ((value & 0x000000FF) << 24);
}


unsigned int big_to_little_endian_add(const char *address) {
    unsigned int value = ((unsigned int)address[0] << 24) |
                         ((unsigned int)address[1] << 16) |
                         ((unsigned int)address[2] << 8) |
                         (unsigned int)address[3];
    return value;
}


void parse_new_node(char *address, char *string_address, char *target, void (*callback)(char *))
{
    while (*(address) == DT_BEGIN_NODE_TOKEN)
    {
        // skip name of the node
        while (*(address) != NULL){
            address++;
        }

        while (*(address) == NULL){
            address++;
        }

        // properties (attributes)
        while (*address == DT_PROP_TOKEN)
        {
            address++;

            // get the length of attribute
            int len = big_to_little_endian_add(address);
            address += 4;

            // get the length to find the target attribute
            int temp = big_to_little_endian_add(address);
            address += 4;

            // if the attribute is correct, get the attribute address
            if (strcmp(string_address + temp, target) == 0)
            {
                callback((char *)big_to_little_endian_add(address));
                uart_puts("found initrd!");
                uart_puts("\n");
                uart_send('\r');
            }

            // jump the value of the attribute
            address += len;
            while (*(address) == NULL){
                address++;
            }
        }

        // children
        parse_new_node(address, string_address, target, callback);
    }

    // go to end
    while (*(address) != DT_END_NODE_TOKEN){
        address++;
    }
    
    address++;

    while (*(address) == NULL){
        address++;
    }

}

char dt_check_magic_number(char *address)
{
    unsigned int magic_number = big_to_little_endian_add(address);

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

    // parse nodes
    while (*(newAddress) != DT_END_TOKEN)
    {
        parse_new_node(newAddress, address + offset_strings, target_property, callback);

        //parse next node
        while (*newAddress != DT_END_NODE_TOKEN) {
            newAddress++;
        }
        newAddress++;
        
        while (*newAddress == NULL) {
            newAddress++;
        }
    }
}