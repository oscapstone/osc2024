#include "main.hpp"

#include "board/mini-uart.hpp"
#include "fdt.hpp"
#include "fs/initramfs.hpp"
#include "fs/vfs.hpp"
#include "int/exception.hpp"
#include "int/interrupt.hpp"
#include "int/irq.hpp"
#include "int/timer.hpp"
#include "io.hpp"
#include "mm/mm.hpp"
#include "mm/mmu.hpp"
#include "sched.hpp"
#include "shell/shell.hpp"
#include "thread.hpp"
#include "util.hpp"

extern char __kernel_beg[];
extern char __kernel_end[];
extern char __kernel_text_beg[];
extern char __kernel_text_end[];
extern char __kernel_data_beg[];
extern char __kernel_data_end[];
extern char __stack_beg[];
extern char __stack_end[];

void kernel_main(void* dtb_addr) {
  mini_uart_setup();
  timer_init();

  klog("Hello Kernel!\n");
  klog("Kernel start   : %p ~ %p\n", __kernel_beg, __kernel_end);
  klog("Exception level: %d\n", get_el());
  klog("freq_of_timer  : %ld\n", freq_of_timer);
  klog("boot time      : " PRTval "s\n", FTval(tick2timeval(boot_timer_tick)));

  fdt.init(pa2va(dtb_addr));
  initramfs::preinit();

  mm_preinit();

  // spin tables for multicore boot
  mm_reserve(pa2va(0x0000), pa2va(0x1000));
  // kernel code & bss & kernel stack
  mm_reserve(__kernel_beg, __kernel_end);
  mm_reserve(__stack_beg, __stack_end);
  // initramfs
  mm_reserve(initramfs::startp(), initramfs::endp());
  // flatten device tree
  mm_reserve(fdt.startp(), fdt.endp());
  // upper page table
  mm_reserve(__upper_PGD, __upper_end);

  mm_init();
  map_kernel_as_normal(__kernel_text_beg, __kernel_text_end);

  irq_init();
  kthread_init();
  schedule_init();
  enable_interrupt();

  mini_uart_use_async(true);
  schedule_timer();

  init_vfs();

  kthread_create(shell);

  idle();
}
