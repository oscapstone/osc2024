#include "initrd.h"

#include <string.h>

#include "uart.h"

char *initrd_base;

int initrd_addr(fdt_node_t node) {
  if (node.tag != FDT_PROP) {
    return 0;
  }

  char *initrd_prop_name = "linux,initrd-start";
  size_t prop_name_size = strnlen(initrd_prop_name, 32);

  if (strncmp(node.prop.prop_name, initrd_prop_name, prop_name_size) != 0) {
    return 0;
  }

  u64_t addr = be2le_32(*(u32_t *)node.prop.prop_val);
  uart_printf("initrd_base: %x\n", addr);
  initrd_base = (char *)addr;

  return 1;
}
