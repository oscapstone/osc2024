#include "dtb.h"
#include "uart.h"
#include "cpio.h"
# include "string.h"

char *cpio_addr;

// rp3 is little endian, but the dtb is big endian
uint32_t uint32_endian_big2lttle(uint32_t data)
{
    char *r = (char*)&data;
    return (r[3]<<0) | (r[2]<<8) | (r[1]<<16) | (r[0]<<24);
}

void send_space(int n) {
	while(n--) uart_puts(" ");
}

void fdt_traverse(void *_dtb_ptr, dtb_callback callback)
{
    fdt_header *header = _dtb_ptr;
    if(uint32_endian_big2lttle(header->magic) != 0xD00DFEED) {
        uart_puts("wrong magic in fdt_traverse: Invalid file format\n");
        return;
    }
    uint32_t struct_size = uint32_endian_big2lttle(header->size_dt_struct);
    char* dt_struct_ptr = (char*)((char*)header + uint32_endian_big2lttle(header->off_dt_struct));
    char* dt_strings_ptr = (char*)((char*)header + uint32_endian_big2lttle(header->off_dt_strings));
    char* end = (char*)dt_struct_ptr + struct_size;
    char* pointer = dt_struct_ptr;

    while(pointer < end)
    {
        uint32_t token_type = uint32_endian_big2lttle(*(uint32_t*)pointer);

        pointer += 4;
        if(token_type == FDT_BEGIN_NODE)
        {
            callback(token_type, pointer, 0, 0);
            pointer += strlen(pointer);
            pointer += 4 - (unsigned long long) pointer % 4;           //alignment 4 byte
        } else if(token_type == FDT_END_NODE)
        {
            callback(token_type, 0, 0, 0);
        } else if(token_type == FDT_PROP)
        {
            uint32_t len = uint32_endian_big2lttle(*(uint32_t*)pointer);
            pointer += 4;
            char* name = (char*)dt_strings_ptr + uint32_endian_big2lttle(*(uint32_t*)pointer);
            pointer += 4;
            callback(token_type, name, pointer, len);
            pointer += len;
            if((unsigned long long)pointer % 4 != 0) pointer += 4 - (unsigned long long)pointer%4;   //alignment 4 byte
        } else if(token_type == FDT_NOP)
        {
            callback(token_type, 0, 0, 0);
        } else if(token_type == FDT_END)
        {
            callback(token_type, 0, 0, 0);
        } else
        {
            uart_puts("error type: ");
            uart_hex(token_type);
            uart_puts("\n");
            return;
        }
    }
}

void show_tree_callback(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
    static int level = 0;
    if(node_type==FDT_BEGIN_NODE)
    {
        send_space(level);
        uart_puts(name);
        uart_puts("{\n");
        level+=4;
    } else if(node_type==FDT_END_NODE)
    {
        level-=4;
        send_space(level);
        uart_puts("}\n");
    } else if(node_type==FDT_PROP)
    {
        send_space(level);
        uart_puts(name);
        uart_puts("\n");
    }
}

// this function is for finding the start address of initramfs 'CPIO_DEFAULT_PLACE'
void initramfs_callback(uint32_t node_type, char *name, void *value, uint32_t name_size) {
    if(node_type==FDT_PROP && strcmp(name,"linux,initrd-start")==1)
    {
        cpio_addr = (char *)uint32_endian_big2lttle(*(uint32_t*)value);
    }
}