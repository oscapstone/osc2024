#include "dtb.h"
#include "mem.h"
#include "shell.h"
#include "utli.h"
extern void *_dtb_ptr_start;

void kernel_init(void *arg) {
  _dtb_ptr_start = arg;
  shell_init();
  fdt_traverse(get_cpio_addr);
  init_mem();
}

void main(void *arg) {
  kernel_init(arg);
  shell_start();
}