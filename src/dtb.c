#include "dtb.h"

#include "cpio_.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"

void *_dtb_ptr;          // should be 0x2EFF7A00
extern char *cpio_addr;  // should be 0x20000000

static unsigned int dtb_strlen(const char *s) {
  unsigned int i = 0;
  while (s[i]) {
    i++;
  }
  return i + 1;
}

static unsigned int fdt_u32_le2be(const void *addr) {
  const unsigned char *bytes = (const unsigned char *)addr;
  unsigned int ret = (unsigned int)bytes[0] << 24 |
                     (unsigned int)bytes[1] << 16 |
                     (unsigned int)bytes[2] << 8 | (unsigned int)bytes[3];
  return ret;
}

static int parse_struct(fdt_callback cb, void *cur_ptr, void *strings_ptr,
                        unsigned int totalsize) {
  void *end_ptr = cur_ptr + totalsize;

  while (cur_ptr < end_ptr) {
    unsigned int token = fdt_u32_le2be(cur_ptr);
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
        unsigned int len = fdt_u32_le2be(cur_ptr);
        cur_ptr += 4;
        unsigned int nameoff = fdt_u32_le2be(cur_ptr);
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

int fdt_traverse(fdt_callback cb) {
  uart_send_string("dtb_ptr address is at: 0x");
  uart_hex((unsigned long int)_dtb_ptr);
  uart_send_string("\r\n");

  fdt_header *header = (fdt_header *)_dtb_ptr;

  unsigned int magic = fdt_u32_le2be(&(header->magic));
  uart_send_string("magic number: 0x");
  uart_hex(magic);
  uart_send_string("\r\n");

  if (magic != 0xd00dfeed) {
    uart_puts("The header magic is wrong");
    return -1;
  }

  void *struct_ptr = _dtb_ptr + fdt_u32_le2be(&(header->off_dt_struct));
  void *strings_ptr = _dtb_ptr + fdt_u32_le2be(&(header->off_dt_strings));
  unsigned int totalsize = fdt_u32_le2be(&(header->totalsize));
  parse_struct(cb, struct_ptr, strings_ptr, totalsize);
  return 1;
};

void get_cpio_addr(int token, const char *name, const void *data,
                   unsigned int size) {
  UNUSED(size);
  if (token == FDT_PROP && !strcmp((char *)name, "linux,initrd-start")) {
    cpio_addr = (char *)(unsigned long int)fdt_u32_le2be(data);
    uart_send_string("cpio address is at: 0x");
    uart_hex((unsigned long int)cpio_addr);
    uart_send_string("\r\n");
  }
  return;
}