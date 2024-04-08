#include "fdt.h"
#include "initrd.h"
#include "shell.h"
#include "uart.h"
#include "utils.h"

void memncpy(void *src, void *dest, size_t n) {
  char *c_src = (char *)src;
  char *c_dest = (char *)dest;

  for (size_t i = 0; i < n; i++) {
    if (c_src[i] == '\0') return;
    c_dest[i] = c_src[i];
  }
}

void main(char *dtb_ptr) {
  /* set up serial console */
  uart_init();

  if (fdt_traversal(dtb_ptr, initrd_addr) < 0) {
    prog_hang();
  }

  /* say hello */
  uart_println("Hello World from boot");

  /* register commands */
  shell_t s;
  register_cmds(&s);

  shell_loop(&s);
}
