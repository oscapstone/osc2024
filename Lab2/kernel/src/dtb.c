#include "dtb.h"
#include "uart.h"
#include <stdint.h>

int32_t b2l_32(int32_t input)
{
    u_32 t;
    unsigned char tmp;
    t.u32 = input;
    for (int i = 0; i < 2; i++)
    {
        tmp = t.u8[i];
        t.u8[i] = t.u8[3 - i];
        t.u8[3 - i] = tmp;
    }
    return t.u32;
}

int64_t b2l_64(int64_t input)
{
    u_64 t;
    unsigned char tmp;
    t.u64 = input;
    // Swap
    for (int i = 0; i < 4; i++)
    {
        tmp = t.u8[i];
        t.u8[i] = t.u8[7 - i];
        t.u8[7 - i] = tmp;
    }
    return t.u64;
}

int fdt_header_b2l(fdt_header *dtb_info)
{
    // If the header is already little endian, ignore it.
    if (dtb_info->magic == FDT_MAGIC)
        return 0;

    dtb_info->magic = b2l_32(dtb_info->magic);
    dtb_info->totalsize = b2l_32(dtb_info->totalsize);
    dtb_info->off_dt_struct = b2l_32(dtb_info->off_dt_struct);
    dtb_info->off_dt_strings = b2l_32(dtb_info->off_dt_strings);
    dtb_info->off_mem_rsvmap = b2l_32(dtb_info->off_mem_rsvmap);
    dtb_info->version = b2l_32(dtb_info->version);
    dtb_info->last_comp_version = b2l_32(dtb_info->last_comp_version);
    dtb_info->boot_cpuid_phys = b2l_32(dtb_info->boot_cpuid_phys);
    dtb_info->size_dt_struct = b2l_32(dtb_info->size_dt_struct);
    dtb_info->size_dt_strings = b2l_32(dtb_info->size_dt_strings);
    return 0;
}

int fdt_find_do(void *dtb_start, const char *name, int (*fn)(void *, int))
{
    char *cur = (char *)dtb_start;
    uint32_t off_struct;
    uint32_t off_string;
    uint32_t size_struct;

    // Get the header information.
    fdt_header *dtb_info = (fdt_header *)dtb_start;

    // Big to little endian
    fdt_header_b2l(dtb_info);

    off_struct = dtb_info->off_dt_struct;
    off_string = dtb_info->off_dt_strings;
    size_struct = dtb_info->size_dt_struct;

    // Traverse
    char *str_lo = cur + off_string;
    cur = cur + off_struct;
    uint32_t *tag;       // Struct tag
    uint32_t t_tag;      // tmp tag, don't change the content in dtb
    fdt_property *prop;  // property tag
    fdt_property t_prop; // tmp prop
    int pad = 0;

    while (1)
    {
        tag = (uint32_t *)cur;
        t_tag = b2l_32(*tag);
        switch (t_tag)
        {
        // begin node
        case FDT_BEGIN_NODE:
            cur += 4;
            pad = 0;
            if (*cur == 0)
            { // IF no name, Add 4-bytes.
                cur += 4;
                break;
            }
            while (*cur)
            {
                cur++;
                pad++;
            }
            cur += 1; // End of string ('\0');
            pad += 1;
            pad = (4 - (pad % 4)) % 4; // Padding to 4-byte
            cur += pad;
            break;
            // end node
        case FDT_END_NODE:
            cur += 4; // Jump to next record
            break;

            // property node
        case FDT_PROP:
            cur += 4;
            prop = (fdt_property *)cur;
            // name offset from string block
            t_prop.nameoff = b2l_32(prop->nameoff);
            t_prop.len = b2l_32(prop->len);
            cur += 8;
            if (!strcmp(str_lo + t_prop.nameoff, name))
            {
                fn((void *)cur, t_prop.len);
                return 0;
            }
            if (prop->len == 0)
            {
                break;
            }
            cur += t_prop.len;
            cur += (4 - (t_prop.len % 4)) % 4; // Padding to 4.
            break;

            // END
        case FDT_END:
            return 0;
            break;

        case FDT_NOP:
            cur += 4; // Ignore
            break;

        default:
            cur += 4; // Error!
        }
    }
    uart_puts(name);
    uart_puts(" Cannot Find!!\n");
    return 1;
}
