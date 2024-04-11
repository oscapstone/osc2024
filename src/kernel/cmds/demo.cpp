#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "interrupt.hpp"
#include "string.hpp"
#include "timer.hpp"

int demo_preempt() {
  auto print_data = [](void* ctx) {
    mini_uart_printf("print from task %ld\n", (long)ctx);
  };

  save_DAIF();
  disable_interrupt();

  add_timer(freq_of_timer, (void*)1, print_data, 1);
  add_timer(freq_of_timer, (void*)0, print_data, 0);
  add_timer(freq_of_timer, (void*)2, print_data, 2);

  restore_DAIF();

  return 0;
}

int cmd_demo(int argc, char* argv[]) {
  if (argc <= 1) {
    mini_uart_puts("demo: require at least one argument\n");
    return -1;
  }

  auto cmd = argv[1];
  if (!strcmp(cmd, "preempt"))
    return demo_preempt();

  mini_uart_printf("demo: task '%s' not found\n", cmd);
  return -1;
}
