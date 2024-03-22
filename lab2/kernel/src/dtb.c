#include "dtb.h"
#include "uart1.h"
#include "u_string.h"
#include "cpio.h"

extern void* CPIO_DEFAULT_PLACE; // it's from shell.c
char* dtb_ptr;

//stored as big endian
struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

uint32_t uint32_endian_big2lttle(uint32_t data)
{
    char* r = (char*)&data;
    return (r[3]<<0) | (r[2]<<8) | (r[1]<<16) | (r[0]<<24);
}

void traverse_device_tree(void *dtb_ptr, dtb_callback callback)
{
    struct fdt_header* header = dtb_ptr;
    if(uint32_endian_big2lttle(header->magic) != 0xD00DFEED)
    {
        uart_puts("traverse_device_tree : wrong magic in traverse_device_tree");
        return;
    }
    // https://abcamus.github.io/2016/12/28/uboot%E8%AE%BE%E5%A4%87%E6%A0%91-%E8%A7%A3%E6%9E%90%E8%BF%87%E7%A8%8B/
    // https://blog.csdn.net/wangdapao12138/article/details/82934127
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
        }else if(token_type == FDT_END_NODE)
        {
            callback(token_type, 0, 0, 0);
        }else if(token_type == FDT_PROP)
        {
            uint32_t len = uint32_endian_big2lttle(*(uint32_t*)pointer);
            pointer += 4;
            char* name = (char*)dt_strings_ptr + uint32_endian_big2lttle(*(uint32_t*)pointer);
            pointer += 4;
            callback(token_type, name, pointer, len);
            pointer += len;
            if((unsigned long long)pointer % 4 != 0) pointer += 4 - (unsigned long long)pointer%4;   //alignment 4 byte
        }else if(token_type == FDT_NOP)
        {
            callback(token_type, 0, 0, 0);
        }else if(token_type == FDT_END)
        {
            callback(token_type, 0, 0, 0);
        }else
        {
            uart_puts("error type:%x\n",token_type);
            return;
        }
    }
}

void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
    static int level = 0;
    if(node_type==FDT_BEGIN_NODE)
    {
        for(int i=0;i<level;i++)uart_puts("   ");
        uart_puts("%s{\n",name);
        level++;
    }else if(node_type==FDT_END_NODE)
    {
        level--;
        for(int i=0;i<level;i++)uart_puts("   ");
        uart_puts("}\n");
    }else if(node_type==FDT_PROP)
    {
        for(int i=0;i<level;i++)uart_puts("   ");
        uart_puts("%s\n",name);
    }
}

// this function is for finding the start address of initramfs 'CPIO_DEFAULT_PLACE'
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size) {
    // https://github.com/stweil/raspberrypi-documentation/blob/master/configuration/device-tree.md
    // linux,initrd-start will be assigned by start.elf based on config.txt
    if(node_type==FDT_PROP && strcmp(name,"linux,initrd-start")==0)
    {
        CPIO_DEFAULT_PLACE = (void *)(unsigned long long)uint32_endian_big2lttle(*(uint32_t*)value);
        uart_puts("initramfs start at 0x%x\n", CPIO_DEFAULT_PLACE);
    }
}
