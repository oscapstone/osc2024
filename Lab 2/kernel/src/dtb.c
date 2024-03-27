#include "dtb.h"
#include "memory.h"
#include "string.h"
#include "uart.h"
#include "initrd.h"


/*
    Flatten Devicetree (DTB) Format

    .dtb (devicetree blob) structure

    +----------------------------+
    |     struct fdt_header      |   
    +----------------------------+
    |        (free space)        |
    +----------------------------+   
    |  memory reservation block  |
    +----------------------------+ 
    |        (free space)        |
    +----------------------------+ 
    |      structure block       |
    +----------------------------+ 
    |        (free space)        |
    +----------------------------+ 
    |        strings block       |
    +----------------------------+ 
    |        (free space)        |
    +----------------------------+ 
*/


typedef struct fdt_header
{
    uint32_t magic;             // contain the value 0xd00dfeed (big-endian).
    uint32_t totalsize;         // in byte
    uint32_t off_dt_struct;     // the offset in bytes of the structure block 
    uint32_t off_dt_strings;    // the offset in bytes of the string block
    uint32_t off_mem_rsvmap;    // the offset in bytes of the memory reservation block
    uint32_t version;           // the version of the devicetree data structure (The version is 17 in the document)
    uint32_t last_comp_version; // the lowest version with which the version used is backwards compatible. (17 to 16)
    uint32_t boot_cpuid_phys;   // the physical ID of the system's boot CPU
    uint32_t size_dt_strings;   // the length in bytes of the strings block section
    uint32_t size_dt_struct;    // the length in bytes of the structure block section
} fdt_header_t;

typedef fdt_header_t* fdt_header_ptr_t;


/*
    convert from 32-bit big-endian to little-endian 
*/
static uint32_t
_dtb_get_uint32(const byteptr_t p)
{
    return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}


/*
    Strcuture block

    a sequence of 32-bit aligned {token, data}

    1. FDT_BEGIN_NODE  0x00000001
        (name) : string

    2. FDT_END_NODE    0x00000002
        (no data)

    3. FDT_PROP        0x00000003
        struct {
            uint32_t len;           // the length of the property's value
            uint32_t nameoff;       // the offset into strings block at which the property's name is stored 
        }
        (value)

    4. FDT_NOP         0x00000004
        (no data)

    5. FDT_END         0x00000009
        (no data)
        it should be the only one and be the last token in the structure block

*/


static uint32_t
_struct_iteration(fdt_callback cb, const byteptr_t struct_ptr, const byteptr_t string_ptr, uint32_t struct_size)
{
    byteptr_t cur_ptr = struct_ptr;
    byteptr_t end_ptr = cur_ptr + struct_size;

    while (cur_ptr < end_ptr) {
        uint32_t token = _dtb_get_uint32(cur_ptr);
        switch (token) {
            case FDT_BEGIN_NODE:
                byteptr_t name_ptr = cur_ptr + 4;
                cb(token, name_ptr, 0, 0);
                cur_ptr = memory_align(name_ptr + str_len(name_ptr) + 1, 4);  // the string length should contain '\0'
                break;
            case FDT_PROP:
                uint32_t data_len = _dtb_get_uint32(cur_ptr + 4);
                uint32_t name_off = _dtb_get_uint32(cur_ptr + 8);
                byteptr_t data_ptr = cur_ptr + 12;
                cb(token, string_ptr + name_off, data_ptr, data_len);
                cur_ptr = memory_align(data_ptr + data_len, 4);
                break;
            case FDT_END_NODE:
            case FDT_NOP:
                cb(token, 0, 0, 0);
                cur_ptr += 4;
                break;
            case FDT_END:
                cb(token, 0, 0, 0);
                return FDT_TRAVERSE_CORRECT;
                break;
            default:
                return FDT_TRAVERSE_FORMAT_ERROR;
        }
    }
    return FDT_TRAVERSE_CORRECT;
}


static byteptr_t _dtb_ptr = nullptr;

byteptr_t
dtb_get_ptr()
{
    return _dtb_ptr;
}

void
dtb_set_ptr(byteptr_t ptr)
{
    _dtb_ptr = ptr;
}


uint32_t
fdt_traverse(fdt_callback cb)
{
    byteptr_t dtb_ptr = dtb_get_ptr();
    fdt_header_ptr_t header = (fdt_header_ptr_t) dtb_ptr;

    if (_dtb_get_uint32((byteptr_t) &(header->magic)) != 0xd00dfeed) {
        return FDT_TRAVERSE_HEADER_ERROR;
    } 

    uint32_t  struct_size = _dtb_get_uint32((byteptr_t) &(header->size_dt_struct));
    byteptr_t struct_ptr  = dtb_ptr + _dtb_get_uint32((byteptr_t) &(header->off_dt_struct));
    byteptr_t string_ptr  = dtb_ptr + _dtb_get_uint32((byteptr_t) &(header->off_dt_strings));

    return _struct_iteration(cb, struct_ptr, string_ptr, struct_size);
}


void 
fdt_find_initrd_addr(uint32_t token, const byteptr_t name_ptr, const byteptr_t data_ptr, uint32_t v)
{
    if (token == FDT_PROP && str_eql(name_ptr, "linux,initrd-start")) {
        byteptr_t _addr = (byteptr_t) (0l | _dtb_get_uint32(data_ptr));
        initrd_set_ptr(_addr);
    }
}


static void 
_print_space(uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) uart_put(' '); 
}

void 
fdt_print_node(uint32_t token, const byteptr_t name_ptr, const byteptr_t, uint32_t)
{
    static uint32_t _level = 0;

    switch (token) {
        case FDT_BEGIN_NODE:
            _print_space(_level);
            uart_str("FDT_BEGIN_NODE ");
            uart_line(name_ptr);
            _level += 2;
            break;
        case FDT_PROP:
            _print_space(_level);
            uart_str("FDT_PROP ");
            uart_line(name_ptr);
            break;
        case FDT_END_NODE:
            _level -= 2;
            _print_space(_level);
            uart_line("FDT_END_NODE");
            break;        
        case FDT_NOP:
            _print_space(_level);
            uart_line("FDT_NOP");
            break;
        case FDT_END:
            _print_space(_level);
            uart_line("FDT_END");
            _level = 0;
            break;
        default:
            break;
    }
}
