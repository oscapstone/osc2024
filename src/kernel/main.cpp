#include "main.hpp"

#include "board/mini-uart.hpp"
#include "fdt.hpp"
#include "fs/initramfs.hpp"
#include "int/exception.hpp"
#include "int/interrupt.hpp"
#include "int/irq.hpp"
#include "int/timer.hpp"
#include "mm/page_alloc.hpp"
#include "mm/startup.hpp"
#include "shell/shell.hpp"

void kernel_main(void* dtb_addr, uint32_t kernel_size) {
  mini_uart_setup();
  mini_uart_puts("Hello Kernel!\n");
  mini_uart_printf("Kernel size: %u\n", kernel_size);
  mini_uart_printf("Kernel start: %p\n", _start);
  mini_uart_printf("Exception level: %d\n", get_el());

  startup_alloc_init();
  fdt.init(dtb_addr);
  page_alloc.init(0x1000'0000, 0x2000'0000);
  initramfs_init();

  timer_init();
  irq_init();
  enable_interrupt();
  mini_uart_printf("freq_of_timer: %ld\n", freq_of_timer);
  mini_uart_printf("boot time    : " PRTval "s\n",
                   FTval(tick2timeval(boot_timer_tick)));

  mini_uart_use_async(true);

  shell();
}
