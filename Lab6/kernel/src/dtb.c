#include "dtb.h"
#include "bool.h"
#include "cpio.h"
#include "memory.h"
#include "mini_uart.h"
#include "string.h"

/* Devicetree Blob (DTB) Format
 * https://github.com/devicetree-org/devicetree-specification/releases/download/v0.4/devicetree-specification-v0.4.pdf
 */

/*  Flattened device tree format
 *  Devicetree .dtb Structure
 *
 * +--------------------------+
 * |     struct fdt_header    |
 * +--------------------------+
 * |       (free space)       |
 * +--------------------------+
 * | memory reservation block |
 * +--------------------------+
 * |       (free space)       |
 * +--------------------------+
 * |      structure block     |
 * +--------------------------+
 * |       (free space)       |
 * +--------------------------+
 * |       strings block      |
 * +--------------------------+
 * |       (free space)       |
 * +--------------------------+
 */

/*
 * Flattened Devicetree Header Fields
 * All the header fields are 32-bit integers,
 * stored in big-endian format.
 */
struct fdt_header {
    uint32_t magic;  // 0xd00dfeed
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

static uintptr_t _dtb_start;
static uintptr_t _dtb_end;

static uintptr_t struct_ptr;
static uintptr_t string_ptr;
static uint32_t struct_size;
static uint32_t total_size;

uintptr_t get_dtb_start(void)
{
    return _dtb_start;
}

void set_dtb_start(uintptr_t ptr)
{
    _dtb_start = ptr;
}

uintptr_t get_dtb_end(void)
{
    return _dtb_end;
}

void set_dtb_end(uintptr_t ptr)
{
    _dtb_end = ptr;
}

static uint32_t dtb_uint32_be2le(const void* addr)
{
    const uint8_t* bytes = (const uint8_t*)addr;
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}

static uint64_t dtb_uint64_be2le(const void* addr)
{
    const uint8_t* bytes = (const uint8_t*)addr;
    return (uint64_t)bytes[0] << 56 | (uint64_t)bytes[1] << 48 |
           (uint64_t)bytes[2] << 40 | (uint64_t)bytes[3] << 32 |
           (uint64_t)bytes[4] << 24 | (uint64_t)bytes[5] << 16 |
           (uint64_t)bytes[6] << 8 | (uint64_t)bytes[7];
}

int fdt_init(uintptr_t dtb_start)
{
    set_dtb_start(dtb_start);

    char* dtb_ptr = (char*)get_dtb_start();
    struct fdt_header* header = (struct fdt_header*)dtb_ptr;

    if (dtb_uint32_be2le(&(header->magic)) != FDT_HEADER_MAGIC)
        return FDT_HEADER_MAGIC_ERROR;

    struct_ptr =
        (uintptr_t)(dtb_ptr + dtb_uint32_be2le(&(header->off_dt_struct)));
    string_ptr =
        (uintptr_t)(dtb_ptr + dtb_uint32_be2le(&(header->off_dt_strings)));

    struct_size = dtb_uint32_be2le(&(header->size_dt_struct));
    total_size = dtb_uint32_be2le(&(header->totalsize));
    set_dtb_end(dtb_start + total_size);

    return 0;
}

/*
 * Structure Block
 *
 * a sequence of 32-bit aligned {token, data} pairs
 * each token is a 32-bit big-endian integer
 *
 * FDT_BEGIN_NODE: 0x00000001
 *  (name) null-terminated string
 *
 * FDT_END_NODE:   0x00000002
 *  (no data)
 *
 * FDT_PROP:       0x00000003
 *  struct {
 *     uint32_t len;      // length of the property's value in bytes
 *     uint32_t nameoff;  // offset into strings block at which the property's
 *                        // name is stored
 *  }
 *  (property's value) len bytes
 *
 * FDT_NOP:        0x00000004
 *  (no data)
 *
 * FDT_END:        0x00000009
 *  (no data)
 *  There shall be only one and it shall be the last token in the block.
 */

static int fdt_struct_parse(fdt_callback cb)
{
    char* cur_ptr = (char*)struct_ptr;
    char* end_ptr = cur_ptr + struct_size;
    char* str_ptr = (char*)string_ptr;

    while (cur_ptr < end_ptr) {
        uint32_t token = dtb_uint32_be2le(cur_ptr);
        switch (token) {
        case FDT_BEGIN_NODE:
            char* name_ptr = cur_ptr + 4;
            cb(token, name_ptr, NULL, 0);
            cur_ptr = mem_align(name_ptr + str_len(name_ptr) + 1, 4);
            break;

        case FDT_PROP:
            uint32_t len = dtb_uint32_be2le(cur_ptr + 4);
            uint32_t name_off = dtb_uint32_be2le(cur_ptr + 8);
            char* data_ptr = cur_ptr + 12;
            cb(token, str_ptr + name_off, data_ptr, len);
            cur_ptr = mem_align(data_ptr + len, 4);
            break;

        case FDT_END_NODE:
        case FDT_NOP:
            cb(token, NULL, NULL, 0);
            cur_ptr += 4;
            break;

        case FDT_END:
            cb(token, NULL, NULL, 0);
            return FDT_TRAVERSE_SUCCESS;

        default:
            return FDT_TRAVERSE_ERROR;
        }
    }

    return FDT_TRAVERSE_ERROR;
}

uint32_t fdt_traverse(fdt_callback cb)
{
    return fdt_struct_parse(cb);
}

void fdt_find_cpio_ptr(uint32_t token,
                       const char* name,
                       const void* data,
                       uint32_t UNUSED(size))
{
    switch (token) {
    case FDT_PROP:
        if (!str_cmp(name, "linux,initrd-start"))
            set_cpio_start((uintptr_t)dtb_uint32_be2le(data) + VA_START);
        else if (!str_cmp(name, "linux,initrd-end"))
            set_cpio_end((uintptr_t)dtb_uint32_be2le(data) + VA_START);
        break;

    default:
        break;
    }
}

static uint32_t level;
void print_dtb(uint32_t token,
               const char* name,
               const void* UNUSED(data),
               uint32_t UNUSED(size))
{
    switch (token) {
    case FDT_BEGIN_NODE:
        uart_send_string("\n");
        uart_send_space_level(level++);
        uart_send_string(name);
        uart_send_string(" {\n");
        break;

    case FDT_END_NODE:
        uart_send_string("\n");
        level--;
        if (level > 0)
            uart_send_space_level(level);
        uart_send_string("}\n");
        break;

    case FDT_PROP:
        uart_send_space_level(level);
        uart_send_string(name);
        break;

    default:
        break;
    }
}

/* find the #address-cells and #size-cells property of root node */
static bool found_root_node = false;

//  number of <u32> cells to represent the address in the reg property in
//  children of root
static uint32_t reg_addr_cells;

// number of <u32> cells to represent the size in the reg property in children
// of root
static uint32_t reg_size_cells;

void fdt_find_root_node(uint32_t token,
                        const char* name,
                        const void* data,
                        uint32_t UNUSED(size))
{
    switch (token) {
    case FDT_BEGIN_NODE:
        if (!str_cmp(name, ""))
            found_root_node = true;
        break;

    case FDT_PROP:
        if (!found_root_node)
            return;

        if (!str_cmp(name, "#address-cells"))
            reg_addr_cells = dtb_uint32_be2le(data);
        else if (!str_cmp(name, "#size-cells"))
            reg_size_cells = dtb_uint32_be2le(data);

        break;

    case FDT_END_NODE:
        found_root_node &= 0;
        break;

    default:
        break;
    }
}

/* find usable memory region */
/* only support one memory node for now */
static bool found_memory_node = false;
static uintptr_t usable_start_addr;  // start address of usable memory region
static uintptr_t usable_length;      // length of usable memory region

uintptr_t get_usable_mem_start(void)
{
    return usable_start_addr;
}

uintptr_t get_usable_mem_length(void)
{
    return usable_length;
}

void fdt_find_memory_node(uint32_t token,
                          const char* name,
                          const void* data,
                          uint32_t UNUSED(size))
{
    switch (token) {
    case FDT_BEGIN_NODE:
        if (!str_n_cmp(name, "memory", 6))
            found_memory_node = true;
        break;

    case FDT_PROP:
        if (!found_memory_node)
            return;

        if (!str_cmp(name, "reg")) {
            switch (reg_addr_cells) {
            case 2:  // 64-bit address
                usable_start_addr = dtb_uint64_be2le(data) + VA_START;
                break;
            case 1:  // 32-bit address
                usable_start_addr = dtb_uint32_be2le(data) + VA_START;
                break;
            default:
                break;
            }

            switch (reg_size_cells) {
            case 2:  // 64-bit size
                usable_length = dtb_uint64_be2le(data + reg_addr_cells * 4);
                break;
            case 1:  // 32-bit size
                usable_length = dtb_uint32_be2le(data + reg_addr_cells * 4);
                break;
            default:
                break;
            }
        }
        break;

    case FDT_END_NODE:
        found_memory_node &= 0;
        break;

    default:
        break;
    }
}
