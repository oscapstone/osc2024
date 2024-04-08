#include "initrd.h"

#include <string.h>

#include "uart.h"

char *initrd_base;

int initrd_addr(u32_t token, char *name, fdt_prop_t *prop, void *data) {
  if (token != FDT_PROP) {
    return 0;
  }

  char *initrd_prop_name = "linux,initrd-start";
  size_t prop_name_size = strnlen(initrd_prop_name, 32);

  if (strncmp(name, initrd_prop_name, prop_name_size) != 0) {
    return 0;
  }

  u64_t addr = be2le_32(*(u32_t *)data);
  uart_printf("initrd_base: %x\n", addr);
  initrd_base = (char *)addr;

  return 1;
}
