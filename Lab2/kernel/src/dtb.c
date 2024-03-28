#include "dtb.h"
#include "cpio.h"
#include "utils.h"
#include "type.h"
#include "uart.h"

/* https://abcamus.github.io/2016/12/26/uboot设备树介绍/ */
/* Tree structure to describe the hardware device */

char *dtb_ptr;

uint32_t uint32_endian_big2lttle(uint32_t data)
{
    char *r = (char *)&data;
    return (r[3] << 0) | (r[2] << 8) | (r[1] << 16) | (r[0] << 24);
}

/* Device tree parser */
void traverse_device_tree(dtb_callback callback)
{
    uint64_t *dt_addr = (uint64_t *)DT_ADDR;
    fdt_header *header = (fdt_header *)*dt_addr;

    uint32_t struct_size = uint32_endian_big2lttle(header->size_dt_struct);
    char *dt_struct_ptr = (char *)((char *)header + uint32_endian_big2lttle(header->off_dt_struct));
    char *dt_strings_ptr = (char *)((char *)header + uint32_endian_big2lttle(header->off_dt_strings));

    char *struct_end = (char *)dt_struct_ptr + struct_size;
    char *curr = dt_struct_ptr;

    while (curr < struct_end)
    {
        uint32_t token_type = uint32_endian_big2lttle(*(uint32_t *)curr);

        curr += 4;
        if (token_type == FDT_BEGIN_NODE)
        {
            callback(token_type, curr, 0, 0);
            curr += strlen(curr);
            curr += 4 - (unsigned long long)curr % 4; // alignment 4 byte
        }
        else if (token_type == FDT_END_NODE)
        {
            callback(token_type, 0, 0, 0);
        }
        else if (token_type == FDT_PROP)
        {
            uint32_t len = uint32_endian_big2lttle(*(uint32_t *)curr);
            curr += 4;
            char *name = (char *)dt_strings_ptr + uint32_endian_big2lttle(*(uint32_t *)curr);
            curr += 4;
            callback(token_type, name, curr, len);
            curr += len;
            if ((unsigned long long)curr % 4 != 0) curr += 4 - (unsigned long long)curr % 4; // alignment 4 byte
        }
        else if (token_type == FDT_NOP)
        {
            callback(token_type, 0, 0, 0);
        }
        else if (token_type == FDT_END)
        {
            callback(token_type, 0, 0, 0);
        }
        else
        {
            uart_puts("error type:");
            uart_puts("\n");
            return;
        }
    }
}

void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
    static int level = 0;
    if (node_type == FDT_BEGIN_NODE)
    {
        for (int i = 0; i < level; i++)
            uart_puts("   ");
        uart_puts(name);
        uart_puts("\n");
        level++;
    }
    else if (node_type == FDT_END_NODE)
    {
        level--;
        for (int i = 0; i < level; i++)
            uart_puts("   ");
        uart_puts("\n");
    }
    else if (node_type == FDT_PROP)
    {
        for (int i = 0; i < level; i++)
            uart_puts("   ");
        uart_puts(name);
        uart_puts("\n");
    }
}

/* http://www.wowotech.net/device_model/dt_basic_concept.html */
void *CPIO_DEFAULT_PLACE;
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size)
{
    if (node_type == FDT_PROP && strcmp(name, "linux,initrd-start") == 0)
    {
        CPIO_DEFAULT_PLACE = (void *)(unsigned long long)uint32_endian_big2lttle(*(uint32_t *)value);
    }
}
