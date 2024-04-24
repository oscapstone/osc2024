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

  if (kernel_size == 0)
    kernel_size = 0x8000;

  klog("Hello Kernel!\n");
  klog("Kernel size    : 0x%x\n", kernel_size);
  klog("Kernel start   : %p\n", _start);
  klog("Exception level: %d\n", get_el());
  klog("freq_of_timer  : %ld\n", freq_of_timer);
  klog("boot time      : " PRTval "s\n", FTval(tick2timeval(boot_timer_tick)));

  fdt.init(dtb_addr);
  initramfs_init();

  mm_preinit();

  // 0x0000 ~ 0x1000 ~ heap ~ stack ~ kernel
  mm_reserve(0x0000, (char*)&_start + kernel_size);
  mm_reserve(initramfs.startp(), initramfs.endp());
  mm_reserve(fdt.startp(), fdt.endp());

  mm_init();

  irq_init();
  enable_interrupt();

  mini_uart_use_async(true);

  shell();
}
