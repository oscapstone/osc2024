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
#include "sched.hpp"
#include "shell/shell.hpp"

void kernel_main(void* dtb_addr) {
  mini_uart_setup();
  timer_init();

  klog("Hello Kernel!\n");
  klog("Kernel start   : %p\n", _start);
  klog("Exception level: %d\n", get_el());
  klog("freq_of_timer  : %ld\n", freq_of_timer);
  klog("boot time      : " PRTval "s\n", FTval(tick2timeval(boot_timer_tick)));

  fdt.init(dtb_addr);
  initramfs_init();

  mm_preinit();

  // spin tables for multicore boot
  mm_reserve(0x0000, 0x1000);
  // kernel code & bss & kernel stack
  mm_reserve(_start, __stack_end);
  // initramfs
  mm_reserve(initramfs.startp(), initramfs.endp());
  // flatten device tree
  mm_reserve(fdt.startp(), fdt.endp());

  mm_init();

  irq_init();
  enable_interrupt();

  mini_uart_use_async(true);

  schedule_init();

  create_thread(shell);

  auto foo = +[]() {
    for (int i = 0; i < 10; ++i) {
      kprintf("Thread id: %d %d\n", current_thread()->tid, i);
      delay(1000000);
      schedule();
    }
  };
  for (int i = 0; i < 2; ++i)
    create_thread(foo);

  idle();
}
