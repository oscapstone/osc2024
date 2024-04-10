#include "board/mini-uart.hpp"
#include "board/timer.hpp"
#include "exception.hpp"
#include "fdt.hpp"
#include "heap.hpp"
#include "initramfs.hpp"
#include "interrupt.hpp"
#include "shell.hpp"

extern "C" void kernel_main(void* dtb_addr) {
  mini_uart_setup();
  mini_uart_puts("Hello Kernel!\n");
  mini_uart_printf("Exception level: %d\n", get_el());

  heap_reset();
  fdt.init(dtb_addr);
  initramfs_init();

  core_timer_enable();
  enable_interrupt();
  mini_uart_printf("freq_of_timer: %ld\n", freq_of_timer);
  mini_uart_printf("boot_timer_tick: %ld\n", boot_timer_tick);

  shell();
}
