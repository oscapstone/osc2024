#include "board/mini-uart.hpp"
#include "exception.hpp"
#include "fdt.hpp"
#include "heap.hpp"
#include "initramfs.hpp"
#include "interrupt.hpp"
#include "irq.hpp"
#include "shell.hpp"
#include "timer.hpp"

void print_2s_timer(void* context) {
  if (timer_delay > 0) {
    auto delay = timer_delay;
    timer_delay = 0;
    mini_uart_printf_sync("delay timer %ds\n", delay);
    auto cur = get_current_tick();
    while (get_current_tick() - cur < delay * freq_of_timer)
      NOP;
  }
  if (show_timer)
    mini_uart_printf("[" PRTval "] 2s timer interrupt\n",
                     FTval(get_current_time()));
  add_timer({2, 0}, (void*)context, (Timer::fp)context);
}

extern "C" void kernel_main(void* dtb_addr) {
  mini_uart_setup();
  mini_uart_puts("Hello Kernel!\n");
  mini_uart_printf("Exception level: %d\n", get_el());

  heap_reset();
  fdt.init(dtb_addr);
  initramfs_init();

  timer_init();
  irq_init();
  enable_interrupt();
  mini_uart_printf("freq_of_timer: %ld\n", freq_of_timer);
  mini_uart_printf("boot time    : " PRTval "s\n",
                   FTval(tick2timeval(boot_timer_tick)));

  add_timer({2, 0}, (void*)print_2s_timer, print_2s_timer);

  mini_uart_use_async(true);

  shell();
}
