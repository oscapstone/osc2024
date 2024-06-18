#include "dtb.h"
#include "interrupt.h"
#include "mem.h"
#include "multitask.h"
#include "sd_card.h"
#include "shell.h"
#include "uart1.h"
#include "utli.h"
#include "vfs.h"

extern void *_dtb_ptr_start;

void kernel_init(void *arg) {
  _dtb_ptr_start = (void *)phy2vir((uint64_t)arg);
  shell_init();
  fdt_traverse(get_cpio_addr);
  init_mem();
  sd_init();
  rootfs_init();
  print_cur_sp();
  init_sched_thread();
  print_cur_sp();
  enable_EL0VCTEN();
  core0_timer_interrupt_enable();
  core_timer_enable();
  set_core_timer_int(get_clk_freq() >> 5);
  enable_interrupt();
}

void main(void *arg) {
  kernel_init(arg);
  thread_create(user_thread_exec);
  // thread_create(shell_start);
  idle_task();
}