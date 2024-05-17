#include "main.hpp"

#include "board/mini-uart.hpp"
#include "fdt.hpp"
#include "fs/initramfs.hpp"
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

extern char __phy_upper_PGD[];
extern char __phy_upper_end[];
extern char __kernel_end[];

void kernel_main(void* dtb_addr) {
  mini_uart_setup();
  timer_init();

  klog("Hello Kernel!\n");
  klog("Kernel start   : %p ~ %p\n", _start, __kernel_end);
  klog("Exception level: %d\n", get_el());
  klog("freq_of_timer  : %ld\n", freq_of_timer);
  klog("boot time      : " PRTval "s\n", FTval(tick2timeval(boot_timer_tick)));

  fdt.init(pa2va(dtb_addr));
  initramfs_init();

  mm_preinit();

  // spin tables for multicore boot
  mm_reserve(0x0000, 0x1000);
  // kernel code & bss & kernel stack
  mm_reserve(va2pa(_start), va2pa(__stack_end));
  // initramfs
  mm_reserve(va2pa(initramfs.startp()), va2pa(initramfs.endp()));
  // flatten device tree
  mm_reserve(va2pa(fdt.startp()), va2pa(fdt.endp()));
  // upper page table
  mm_reserve(__phy_upper_PGD, __phy_upper_end);

  mm_init();

  irq_init();
  kthread_init();
  schedule_init();
  enable_interrupt();

  mini_uart_use_async(true);

  kthread_create(shell);

  idle();
}
