#include "dtb_parser.h"
#include "uart.h"
#include "utils.h"
#include "cpio.h"

char *dtb_ptr;
extern unsigned long long cpio_address;

void fdt_traverse(FuncPtr fun_ptr)
{
    struct fdt_header *header = (struct fdt_header *)dtb_ptr;
    if (fdt_u32_le2be(&(header->magic)) != 0xd00dfeed)
    {
        uart_puts("fdt error\n");
        return;
    }

    char *struct_ptr = dtb_ptr + fdt_u32_le2be(&(header->off_dt_struct));
    char *string_ptr = dtb_ptr + fdt_u32_le2be(&(header->off_dt_strings));

    uart_puts("parsing fdt struct...\n");
    struct_parser(fun_ptr, header, struct_ptr, string_ptr);
}

void struct_parser(FuncPtr fun_ptr, struct fdt_header *header, char *struct_ptr, char *string_ptr)
{
    char *cur_ptr = struct_ptr;

    while (1)
    {
        uint32_t token = fdt_u32_le2be(cur_ptr);

        if (token == FDT_BEGIN_NODE)
        {
            uint32_t len = my_strlen(cur_ptr + 4) + 1;

            fun_ptr(token, cur_ptr + 4, 0x0);

            // align to 4
            cur_ptr += 4 + len + (len % 4 == 0 ? 0 : 4 - len % 4);
        }
        else if (token == FDT_END_NODE)
        {
            cur_ptr += 4;

            fun_ptr(token, 0x0, 0x0);
        }
        else if (token == FDT_PROP)
        {
            struct fdt_property *prop_node_ptr = (struct fdt_property *)(cur_ptr + 4);
            uint32_t len = fdt_u32_le2be(&(prop_node_ptr->len));
            uint32_t nameoff = fdt_u32_le2be(&(prop_node_ptr->nameoff));
            
            fun_ptr(token, (char *)(string_ptr + nameoff), (char *)prop_node_ptr + 8);

            // align to 4
            cur_ptr += 4 * 3 + len + (len % 4 == 0 ? 0 : 4 - len % 4);
        }
        else if (token == FDT_NOP)
        {
            cur_ptr += 4;
            fun_ptr(token, 0x0, 0x0);
        }
        else if (token == FDT_END)
        {
            fun_ptr(token, 0x0, 0x0);
            break;
        }
        else
        {
            uart_puts("parse struct error\n");
            return;
        }
    }
}

void initramfs_callback(uint32_t token, char *name, char *prop)
{
    if (my_strcmp(name, "linux,initrd-start") == 0)
    {
        cpio_address = fdt_u32_le2be(prop);
        uart_puts("initrd-start: ");
        uart_hex_lower_case((unsigned int)cpio_address);
        uart_puts("\n");
    }
}

void device_name_callback(uint32_t token, char *name, char *prop)
{
    if (token == FDT_BEGIN_NODE)
    {
        uart_puts("FDT_BEGIN_NODE\n");
        uart_puts(name);
        uart_puts("\n");
    }
    else if (token == FDT_END_NODE)
        uart_puts("FDT_END_NODE\n");
    else if (token == FDT_PROP)
    {
        uart_puts("FDT_PROP\n");
        uart_puts(name);
        uart_puts("\n");
    }
    else if (token == FDT_NOP)
        uart_puts("FDT_NOP\n");
    else if (token == FDT_END)
        uart_puts("FDT_END\n");
    else
        uart_puts("token doesn't exist\n");

}