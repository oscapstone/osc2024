#include "dtb.h"
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
  uint32_t magic; // 0xd00dfeed
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

static uint32_t dtb_uint32_be2le(const void *addr) {
  const uint8_t *bytes = (const uint8_t *)addr;
  return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
         (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
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

static int fdt_struct_parse(fdt_callback cb, char *struct_ptr, char *string_ptr,
                            uint32_t struct_size) {
  char *cur_ptr = struct_ptr;
  char *end_ptr = cur_ptr + struct_size;

  while (cur_ptr < end_ptr) {
    uint32_t token = dtb_uint32_be2le(cur_ptr);
    switch (token) {
    case FDT_BEGIN_NODE:
      char *name_ptr = cur_ptr + 4;
      cb(token, name_ptr, NULL, 0);
      cur_ptr = mem_align(name_ptr + str_len(name_ptr) + 1, 4);
      break;

    case FDT_PROP:
      uint32_t len = dtb_uint32_be2le(cur_ptr + 4);
      uint32_t name_off = dtb_uint32_be2le(cur_ptr + 8);
      char *data_ptr = cur_ptr + 12;
      cb(token, string_ptr + name_off, data_ptr, len);
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

static uintptr_t _dtb_ptr;

uintptr_t get_dtb_ptr(void) { return _dtb_ptr; }

void set_dtb_ptr(uintptr_t ptr) { _dtb_ptr = ptr; }

uint32_t fdt_traverse(fdt_callback cb) {
  char *dtb_ptr = (char *)_dtb_ptr;
  struct fdt_header *header = (struct fdt_header *)dtb_ptr;

  if (dtb_uint32_be2le(&(header->magic)) != FDT_HEADER_MAGIC)
    return FDT_HEADER_MAGIC_ERROR;

  char *struct_ptr = dtb_ptr + dtb_uint32_be2le(&(header->off_dt_struct));
  char *string_ptr = dtb_ptr + dtb_uint32_be2le(&(header->off_dt_strings));
  uint32_t struct_size = dtb_uint32_be2le(&(header->size_dt_struct));

  return fdt_struct_parse(cb, struct_ptr, string_ptr, struct_size);
}

void fdt_find_cpio_ptr(uint32_t token, const char *name, const void *data,
                       uint32_t UNUSED(size)) {
  if (token == FDT_PROP && !str_cmp(name, "linux,initrd-start"))
    set_cpio_ptr((uint64_t)dtb_uint32_be2le(data));
}

static uint32_t level;
void print_dtb(uint32_t token, const char *name, const void *UNUSED(data),
               uint32_t UNUSED(size)) {
  switch (token) {
  case FDT_BEGIN_NODE:
    uart_send_string("\n");
    uart_send_space_level(level++);
    uart_send_string(name);
    uart_send_string("{\n");
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
