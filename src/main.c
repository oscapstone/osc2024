#include "dtb.h"
#include "shell.h"
#include "utli.h"
extern void *_dtb_ptr;

void kernel_init(void *arg) {
  _dtb_ptr = arg;
  shell_init();
  fdt_traverse(get_cpio_addr);
  print_cur_el();
  print_cur_sp();
  
}

void main(void *arg) {
  kernel_init(arg);
  shell_start();
}