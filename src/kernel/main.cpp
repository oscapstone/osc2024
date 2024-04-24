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
#include "shell/shell.hpp"

void kernel_main(void* dtb_addr, uint32_t kernel_size) {
  mini_uart_setup();
  timer_init();

  klog("Hello Kernel!\n");
  klog("Kernel size    : %u\n", kernel_size);
  klog("Kernel start   : %p\n", _start);
  klog("Exception level: %d\n", get_el());
  klog("freq_of_timer  : %ld\n", freq_of_timer);
  klog("boot time      : " PRTval "s\n", FTval(tick2timeval(boot_timer_tick)));

  fdt.init(dtb_addr);
  initramfs_init();

  mm_init();

  irq_init();
  enable_interrupt();

  mini_uart_use_async(true);

  shell();
}
