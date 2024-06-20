#include "dtb.h"
#include "uart.h"
#include "shell.h"

char* cpio_base;
char* cpio_end;
char* dtb_start;
char* dtb_end;
unsigned long memory_start;
unsigned long memory_end;

void initramfs_start_callback(char *address)
{
    cpio_base = address;
}

void initramfs_end_callback(char *address)
{
    cpio_end = address;
}


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

void fdt_tranverse(void * dtb_base, char *target_property, void (*callback)(char *))
{
    struct fdt_header * header = (struct fdt_header *) dtb_base;
    
    unsigned int temp, offset_struct, offset_strings, magic;

    magic = big_to_little_endian(header -> magic);

    if (magic != FDT_MAGIC_NUMBER) //0xD00DFEED
    {
        uart_puts("Invalid device tree\n");
        return;
    }

    unsigned int totalsize = big_to_little_endian(header -> totalsize);

    if(dtb_start == 0){
        dtb_start = dtb_base;
        dtb_end = dtb_start + totalsize;
    }

    //value in dtb is big endian
    offset_struct = big_to_little_endian(header -> off_dt_struct); //the offset to get the structure block
    offset_strings = big_to_little_endian(header -> off_dt_strings); //the offset to get the property name

    char *newAddress = dtb_base + offset_struct;
    char *string_address = dtb_base + offset_strings;
    

    // parse nodes
    while (1)
    {
        unsigned int token = big_to_little_endian_add(newAddress);
        newAddress += 4; //skip token
        //parse_new_node(newAddress, dtb_base + offset_strings, target_property, callback);

        if (token == FDT_BEGIN_NODE_TOKEN){
            int cnt = 0;
            while(*newAddress != NULL){
                cnt++;
                newAddress++;
            }

            int align = (4 -  cnt % 4);            
            if(align != 4)
                newAddress += align;

        }
        else if(token == FDT_PROP_TOKEN){
            
            /*
            struct {
                uint32_t len;
                uint32_t nameoff;
            }*/

            // get the length of attribute
            int len = big_to_little_endian_add(newAddress);
            newAddress += 4;

            // get the length to find the target attribute
            int temp = big_to_little_endian_add(newAddress);
            newAddress += 4;

            if (strcmp(string_address + temp, target_property) == 0)
            {
                /* The /chosen node does not represent a real device in the system but describes parameters chosen or specified by 
                the system firmware at run time. It shall be a child of the root node. */
                callback((char *)big_to_little_endian_add(newAddress));
                uart_puts("found ");
                uart_puts(target_property);
                uart_puts(" in ");
                uart_hex((unsigned int)newAddress);
                uart_puts("\n");
                uart_send('\r');
            }

            // jump the value of the attribute
            newAddress += len;
            int align = (4 -  len % 4);            
            if(align != 4)
                newAddress += align;

        }
        else if(token == FDT_END_TOKEN)
            break;
    }
}

void get_memory(void * dtb_base)
{
    int lookfor = 0;
    struct fdt_header * header = (struct fdt_header *) dtb_base;
    
    unsigned int temp, offset_struct, offset_strings, magic;

    magic = big_to_little_endian(header -> magic);

    if (magic != FDT_MAGIC_NUMBER) //0xD00DFEED
    {
        uart_puts("Invalid device tree\n");
        return;
    }

    unsigned int totalsize = big_to_little_endian(header -> totalsize);

    if(dtb_start == 0){
        dtb_start = dtb_base;
        dtb_end = dtb_start + totalsize;
    }

    //value in dtb is big endian
    offset_struct = big_to_little_endian(header -> off_dt_struct); //the offset to get the structure block
    offset_strings = big_to_little_endian(header -> off_dt_strings); //the offset to get the property name

    char *newAddress = dtb_base + offset_struct;
    char *string_address = dtb_base + offset_strings;
    

    // parse nodes
    while (1)
    {
        unsigned int token = big_to_little_endian_add(newAddress);
        newAddress += 4; //skip token
        //parse_new_node(newAddress, dtb_base + offset_strings, target_property, callback);

        if (token == FDT_BEGIN_NODE_TOKEN){
            int cnt = 0;
            while(*newAddress != NULL){
                cnt++;
                newAddress++;
            }

            int align = (4 -  cnt % 4);            
            if(align != 4)
                newAddress += align;

        }
        else if(token == FDT_PROP_TOKEN){
            
            /*
            struct {
                uint32_t len;
                uint32_t nameoff;
            }*/

            // get the length of attribute
            int len = big_to_little_endian_add(newAddress);
            newAddress += 4;
            
            // get the length to find the target attribute
            int temp = big_to_little_endian_add(newAddress);
            newAddress += 4;

            if (strcmp(string_address + temp, "device_type") == 0)
            {
                if(strcmp(newAddress, "memory") == 0){
                    lookfor = 1;
                }
            }

            if(strcmp(string_address + temp, "reg") == 0 && lookfor){

                uart_puts("Memory regions:\n");
                // Assuming each address and size is 4 bytes for simplicity:
                memory_start = big_to_little_endian_add(newAddress);
                newAddress += 4;
                memory_end = big_to_little_endian_add(newAddress);
                newAddress += 4;
                uart_puts("Start = ");
                uart_hex(memory_start);
                uart_puts(", End = ");
                uart_hex(memory_end);
                uart_puts("\n");
                lookfor = 0;
            }

            // jump the value of the attribute
            newAddress += len;
            int align = (4 -  len % 4);            
            if(align != 4)
                newAddress += align;

        }
        else if(token == FDT_END_TOKEN)
            break;
    }
}