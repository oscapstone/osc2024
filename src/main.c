#include "dtb.h"
#include "interrupt.h"
#include "mem.h"
#include "multitask.h"
#include "shell.h"
#include "syscall_.h"
#include "uart1.h"
#include "utli.h"
extern void *_dtb_ptr_start;

void kernel_init(void *arg) {
  _dtb_ptr_start = arg;
  shell_init();
  fdt_traverse(get_cpio_addr);
  init_mem();
  print_cur_sp();
  init_sched_thread();
  print_cur_sp();
  enable_EL0VCTEN();
  core0_timer_interrupt_enable();
  core_timer_enable();
  set_core_timer_int(get_clk_freq() >> 5);
  enable_uart_interrupt();
  enable_interrupt();
}

void main(void *arg) {
  kernel_init(arg);
  startup_thread_exec("syscall.img");
  shell_start();
}