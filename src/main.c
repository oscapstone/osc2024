#include "dtb.h"
#include "interrupt.h"
#include "mem.h"
#include "shell.h"
#include "utli.h"
extern void *_dtb_ptr_start;

void kernel_init(void *arg) {
  _dtb_ptr_start = arg;
  shell_init();
  fdt_traverse(get_cpio_addr);
  init_mem();
  enable_EL0VCTEN();
  enable_interrupt();
  core_timer_enable();
  set_core_timer_int_sec(1);
  core0_timer_interrupt_enable();
  // print_cur_el();
  // print_cur_sp();
}

void main(void *arg) {
  kernel_init(arg);
  shell_start();
}