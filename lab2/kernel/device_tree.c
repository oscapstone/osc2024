#include "device_tree.h"
#include "../lib/utils.h"
#include "bsp/uart.h"
#include "io.h"
#include <string.h>

// get this value from start.S
extern uint64_t *dtb_base;

void fdt_traverse(void (*callback)(void *)) {
  struct fdt_header *header = (struct fdt_header *)(dtb_base);
  print_string("Dtb loaded address: ");
  uart_hex(*(uint32_t *)header);
  print_string("\n");

  // get the offset of the structure block and string block by adding the base
  // address of the header to the offset
  uint32_t *structure = (uint32_t *)header + be2le(&header->off_dt_struct);
  uint32_t *strings = (uint32_t *)header + be2le(&header->off_dt_strings);

  print_string("Structure block address: ");
  uart_hex((uint32_t)structure);

  print_string("\nString block address: ");
  uart_hex((uint32_t)strings);

  uint32_t totalsize = be2le(&header->totalsize);

  // Parse the structure block
  uint32_t *ptr = structure; // Point to the beginning of structure block
  while (ptr < strings + totalsize) {
    uint32_t token = be2le((char *)ptr);
    ptr += 4; // Token takes 4 bytes

    switch (token) {
    case FDT_BEGIN_NODE:
      print_string((char *)ptr);
      ptr += align4(strlen((char *)ptr));
      break;
    case FDT_END_NODE:
      break;
    case FDT_PROP: {
      uint32_t len = be2le((char *)ptr);
      ptr += 4;
      uint32_t nameoff = be2le((char *)ptr);
      ptr += 4;
      if (!strcmp((char *)(strings + nameoff), "linux,initrd-start")) {
        callback((void *)(uint32_t *)be2le((void *)ptr));
      }
      ptr += align4(len);
      break;
    }
    case FDT_NOP:
      break;
    case FDT_END:
      break;
    }
  }
}

static uint32_t be2le(const void *s) {
  const uint8_t *bytes = (const uint8_t *)s;
  return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
         (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}
