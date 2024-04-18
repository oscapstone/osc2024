#include "dtb.h"
#include "interrupt.h"
#include "mem.h"
#include "multitask.h"
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
  set_core_timer_int(1);
  core0_timer_interrupt_enable();
  init_sched_thread();
}

void main(void *arg) {
  kernel_init(arg);
  shell_start();
}