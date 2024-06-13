#include "bcm2837/rpi_mmu.h"
#include "dtb.h"
#include "uart1.h"
#include "string.h"
#include "cpio.h"
#include "memory.h"

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

// memory reservation block - https://zhuanlan.zhihu.com/p/144863497
struct fdt_reserve_entry {
    uint64_t address;
    uint64_t size;
};

uint32_t uint32_endian_big2lttle(uint32_t data)
{
    char *r = (char *)&data;
    return (r[3] << 0) | (r[2] << 8) | (r[1] << 16) | (r[0] << 24);
}

uint64_t uint64_endian_big2lttle(uint64_t data)
{
    char *r = (char *)&data;
    return ((unsigned long long)r[7] << 0) | ((unsigned long long)r[6] << 8) | ((unsigned long long)r[5] << 16) | ((unsigned long long)r[4] << 24) | ((unsigned long long)r[3] << 32) | ((unsigned long long)r[2] << 40) | ((unsigned long long)r[1] << 48) | ((unsigned long long)r[0] << 56);
}

uint32_t utils_align_up(uint32_t size, int alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

// count the length of string
size_t utils_strlen(const char *s)
{
    size_t i = 0;
    while (s[i])
        i++;
    return i + 1;
}

void traverse_device_tree(void *dtb_ptr, dtb_callback callback)
{
    struct fdt_header *header = dtb_ptr;
    if (uint32_endian_big2lttle(header->magic) != 0xD00DFEED)
    {
        uart_sendline("wrong magic in traverse_device_tree\n");
        return;
    }

    uint32_t struct_size = uint32_endian_big2lttle(header->size_dt_struct);
    char *struct_ptr = (char *)((char *)header + uint32_endian_big2lttle(header->off_dt_struct));
    char *string_ptr = (char *)((char *)header + uint32_endian_big2lttle(header->off_dt_strings));

    char *end = (char *)struct_ptr + struct_size;
    char *pointer = struct_ptr;

    while (pointer < end)
    {
        uint32_t type = uint32_endian_big2lttle(*(uint32_t *)pointer);

        pointer += 4;

        if (type == FDT_BEGIN_NODE)
        {
            callback(type, pointer, 0, 0);
            pointer += utils_align_up(utils_strlen((char *)pointer), 4);
        }
        else if (type == FDT_END_NODE)
        {
            callback(type, 0, 0, 0);
        }
        else if (type == FDT_PROP)
        {
            uint32_t len = uint32_endian_big2lttle(*(uint32_t *)pointer);
            pointer += 4;
            char *name = (char *)string_ptr + uint32_endian_big2lttle(*(uint32_t *)pointer);
            pointer += 4;
            callback(type, name, pointer, len);
            pointer += utils_align_up(len, 4);
        }
        else if (type == FDT_NOP)
        {
            callback(type, 0, 0, 0);
        }
        else if (type == FDT_END)
        {
            callback(type, 0, 0, 0);
        }
        else
        {
            uart_sendline("error type:%x\n", type);
            return;
        }
    }
}

void dtb_cb_print(uint32_t type, char *name, void *data, uint32_t name_size)
{
    static int space = 0;
    if (type == FDT_BEGIN_NODE)
    {
        for (int i = 0; i < space; i++)
            uart_sendline(" ");
        uart_sendline("%s{\n", name);
        space++;
    }
    else if (type == FDT_END_NODE)
    {
        space--;
        for (int i = 0; i < space; i++)
            uart_sendline("   ");
        uart_sendline("}\n");
    }
    else if (type == FDT_PROP)
    {
        for (int i = 0; i < space; i++)
            uart_sendline(" ");
        uart_sendline("%s{\n", name);
    }
}

void dtb_cb_init(uint32_t type, char *name, void *value, uint32_t name_size){
    if (type == FDT_PROP && strcmp(name,"linux,initrd-start") == 0)
    {
        CPIO_START = (void *)(unsigned long long)PHYS_TO_VIRT(uint32_endian_big2lttle(*(uint32_t*)value));
    }
    if (type == FDT_PROP && strcmp(name, "linux,initrd-end") == 0)
    {
        CPIO_END = (void *)(unsigned long long)PHYS_TO_VIRT(uint32_endian_big2lttle(*(uint32_t *)value));
    }
    
}

void dtb_reserved_memory(){
    struct fdt_header *header = (struct fdt_header *) dtb_ptr;
    if (uint32_endian_big2lttle(header->magic) != 0xD00DFEED)
    {
        uart_sendline("wrong magic\n");
        return;
    }

    // off_mem_rsvmap stores all of reserve memory map with address and size
    // off_mem_rsvmap contains offset in bytes of the memory reservation block from the beginning of the header.
    char *mem_rsvmap = (char *)((char *)header + uint32_endian_big2lttle(header->off_mem_rsvmap));
    struct fdt_reserve_entry *reverse_entry = (struct fdt_reserve_entry *)mem_rsvmap;   //[0:63]address + [64:127]size
    // int count = 0;
    // reserve memory which is defined by dtb
    while (reverse_entry->address != 0 || reverse_entry->size != 0)
    {
        // count++;
        //  start address + size = end address
        unsigned long long start = PHYS_TO_VIRT(uint64_endian_big2lttle(reverse_entry->address));
        unsigned long long end = uint64_endian_big2lttle(reverse_entry->size) + start;

        memory_reserve(start, end);
        reverse_entry++;
    }
    // reserve device tree itself
    memory_reserve((unsigned long long)dtb_ptr, (unsigned long long)dtb_ptr + uint32_endian_big2lttle(header->totalsize));
    //  totalsize: the total size in bytes of the devicetree data structure
    // uart_sendline("dtb count = %d",count);
}

/*
The memory reservation block provides the client program with a list of areas in physical memory which are reserved
The list of reserved blocks shall be terminated with an entry where both address and size are equal to 0
*/