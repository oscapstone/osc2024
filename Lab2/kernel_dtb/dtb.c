#include "dtb.h"
#include "uart.h"
#include "shell.h"


struct fdt_header {
    // big endian default
    unsigned int magic;             // Magic word, signifies the start of the FDT blob
    unsigned int totalsize;         // Total size of the FDT blob in bytes
    unsigned int off_dt_struct;     // Offset to the structure block from the beginning of the FDT
    unsigned int off_dt_strings;    // Offset to the strings block from the beginning of the FDT
    unsigned int off_mem_rsvmap;    // Offset to the memory reservation block
    unsigned int version;           // Version of the device tree specification
    unsigned int last_comp_version; // Last compatible version number (lowest version supported)
    unsigned int boot_cpuid_phys;   // Physical CPU ID of the boot processor
    unsigned int size_dt_strings;   // Size of the strings block in bytes
    unsigned int size_dt_struct;    // Size of the structure block in bytes
};

// when hex to binary, two digit become 8 binary, which is a byte of int
// little endian to fit the environment
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
    /*
    Tranverse into every node, get struct which contains len of prop and name address when meet prop. String + name -> property name, len -> property size.
    */
    while (*(address) == FDT_BEGIN_NODE_TOKEN)
    {
        // skip name of the node
        while (*(address) != NULL){
            address++;
        }
        //NULL terminating and align
        while (*(address) == NULL){
            address++;
        }

        // properties of the node
        while (*address == FDT_PROP_TOKEN)
        {
            /*
            struct {
                uint32_t len;
                uint32_t nameoff;
            }
            */
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
                /* The /chosen node does not represent a real device in the system but describes parameters chosen or specified by 
                the system firmware at run time. It shall be a child of the root node. */
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
    while (*(address) != FDT_END_NODE_TOKEN){
        address++;
    }
    
    address++;

    while (*(address) == NULL){
        address++;
    }

}


void fdt_tranverse(char *address, char *target_property, void (*callback)(char *))
{
    
    struct fdt_header * header = (struct fdt_header *) address;
    unsigned int temp;
    unsigned int offset_struct, offset_strings, magic;
    magic = big_to_little_endian(header -> magic);

    if (magic != FDT_MAGIC_NUMBER) //0xD00DFEED
    {
        uart_puts("Invalid device tree\n");
        return;
    }

    //value in dtb is big endian
    offset_struct = big_to_little_endian(header -> off_dt_struct); //the offset to get the structure block
    offset_strings = big_to_little_endian(header -> off_dt_strings); //the offset to get the property name

    char *newAddress = address + offset_struct;
    
    // point to the dtb structure
    while (*newAddress == NULL) {
        newAddress++;
    }

    // parse nodes
    while (*(newAddress) != FDT_END_TOKEN)
    {
        parse_new_node(newAddress, address + offset_strings, target_property, callback);

        //parse next node
        while (*newAddress != FDT_END_NODE_TOKEN) {
            newAddress++;
        }
        newAddress++;
        
        while (*newAddress == NULL) {
            newAddress++;
        }
    }
}