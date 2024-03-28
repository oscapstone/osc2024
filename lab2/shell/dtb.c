#include "headers/dtb.h"
#include "headers/uart.h"
#include "headers/utils.h"

char *cpio_addr;

unsigned int endian_big2little(unsigned int x)
{
    return (x>>24) | ((x>>8) & 0xff00) | ((x<<8) & 0xff0000) | (x<<24);
} 

void fdt_traverse(fdt_callback callback, void *dtb_base)
{
    struct fdt_header *header = (struct fdt_header *)dtb_base;

    if(endian_big2little(header->magic) != (0xD00DFEED))
    {
        display("Wrong magic in fdt header\n");
        return;
    }

    unsigned int struct_size = endian_big2little(header->size_dt_struct);
    char *struct_ptr = (char*)((char*)header + endian_big2little(header->off_dt_struct));
    char *strings_ptr = (char*)((char*)header + endian_big2little(header->off_dt_strings));

    char *end = (char*)struct_ptr + struct_size;
    char *pointer = struct_ptr;

    while(pointer < end)
    {
        unsigned int type = endian_big2little(*(unsigned int*)pointer);
        pointer += 4;

        switch(type)
        {
            case FDT_BEGIN_NODE: // contains a name string
            {   
                pointer += strlen(pointer);
                pointer += (4 - (unsigned int)pointer % 4);
                break;
            }
            case FDT_PROP: // contains len(uint32_t) and nameoff(uint32_t)
            {
                unsigned int len = endian_big2little(*(unsigned int *)pointer);
                pointer += 4;
                char *name = (char*)strings_ptr + endian_big2little(*(unsigned int*)pointer);
                pointer += 4;
                callback(type, name, pointer, len);
                pointer += len;
                if((unsigned int)pointer % 4 != 0)
                    pointer += 4 - (unsigned int)pointer % 4;
                break;
            }
            case FDT_NOP:
            {
                break;
            }
            case FDT_END_NODE:
            {
                break;
            }
        }
    }
}

void initramfs_callback(unsigned int type, char *name, void *value, unsigned int name_size)
{
    if (strcmp(name, "linux,initrd-start"))
    {
        cpio_addr = (char*)(unsigned long long)endian_big2little(*(unsigned int*)value);
        char cpio_addr_str[8];
        bin2hex((unsigned int) cpio_addr, cpio_addr_str);
        display("The cpio_addr is at: 0x"); display(cpio_addr_str); display("\n");
    }
}