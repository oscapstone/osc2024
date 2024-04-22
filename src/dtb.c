#include "dtb.h"

#include "cpio_.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"

void *_dtb_ptr_start;  // should be 0x2EFF7A00
void *_dtb_ptr_end;
extern char *cpio_start_addr;  // should be 0x20000000
extern char *cpio_end_addr;

static uint32_t dtb_strlen(const char *s) {
  uint32_t i = 0;
  while (s[i]) {
    i++;
  }
  return i + 1;
}

static uint32_t fdt_u32_le2be(const void *addr) {
  const uint8_t *bytes = (const uint8_t *)addr;
  uint32_t ret = (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
                 (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
  return ret;
}

static int32_t parse_struct(fdt_callback cb, void *cur_ptr, void *strings_ptr,
                            uint32_t totalsize) {
  void *end_ptr = cur_ptr + totalsize;

  while (cur_ptr < end_ptr) {
    uint32_t token = fdt_u32_le2be(cur_ptr);
    cur_ptr += 4;

    switch (token) {
      case FDT_BEGIN_NODE:
        /*
        Token type (4 bytes): Indicates that it's an FDT_BEGIN_NODE token.
        Node name (variable length, NULL-terminated): Specifies the name of the
        node being opened.
        */
        cb(token, (char *)cur_ptr, (void *)0, 0);
        cur_ptr += align(dtb_strlen((char *)cur_ptr), 4);
        break;
      case FDT_END_NODE:
        /*
        Token type (4 bytes): Indicates that it's an FDT_END_NODE token.
        */
        cb(token, (char *)0, (void *)0, 0);
        break;
      case FDT_PROP: {
        /*
        Token type (4 bytes): Indicates that it's an FDT_PROP token.
        Data length (4 bytes): Specifies the length of the property data (len).
        Name offset (4 bytes): Provides the offset of the property name within
        the strings block (nameoff). Property data (variable length): Contains
        the property data itself, the size of which is determined by len.
        */
        uint32_t len = fdt_u32_le2be(cur_ptr);
        cur_ptr += 4;
        uint32_t nameoff = fdt_u32_le2be(cur_ptr);
        cur_ptr += 4;
        // second parameter name here is property name not node name
        cb(token, (char *)(strings_ptr + nameoff), (void *)cur_ptr, len);
        cur_ptr += align(len, 4);
        break;
      }
      case FDT_NOP:
        cb(token, (char *)0, (void *)0, 0);
        break;
      case FDT_END:
        cb(token, (char *)0, (void *)0, 0);
        return 0;
      default:;
        return -1;
    }
  }
  return -1;
}

/*
   +-----------------+
   | fdt_header      | <- dtb_ptr
   +-----------------+
   | reserved memory |
   +-----------------+
   | structure block | <- dtb_ptr + header->off_dt_struct (struct_ptr)
   +-----------------+
   | strings block   | <- dtb_ptr + header->off_dt_strings (strings_ptr)
   +-----------------+
*/

int32_t fdt_traverse(fdt_callback cb) {
  fdt_header *header = (fdt_header *)_dtb_ptr_start;
  _dtb_ptr_end = _dtb_ptr_start + fdt_u32_le2be(&header->totalsize);
  uint32_t magic = fdt_u32_le2be(&(header->magic));
  uart_send_string("magic number: 0x");
  uart_hex(magic);
  uart_send_string("\r\n");

  if (magic != 0xd00dfeed) {
    uart_puts("The header magic is wrong");
    return -1;
  }

  void *struct_ptr = _dtb_ptr_start + fdt_u32_le2be(&(header->off_dt_struct));
  void *strings_ptr = _dtb_ptr_start + fdt_u32_le2be(&(header->off_dt_strings));
  uint32_t totalsize = fdt_u32_le2be(&(header->totalsize));
  parse_struct(cb, struct_ptr, strings_ptr, totalsize);
  return 1;
};

void get_cpio_addr(int32_t token, const char *name, const void *data,
                   uint32_t size) {
  UNUSED(size);

  if (token == FDT_PROP && !strcmp((char *)name, "linux,initrd-start")) {
    cpio_start_addr = (char *)(uint64_t)fdt_u32_le2be(data);
  }
  if (token == FDT_PROP && !strcmp((char *)name, "linux,initrd-end")) {
    cpio_end_addr = (char *)(uint64_t)fdt_u32_le2be(data);
  }
  return;
}