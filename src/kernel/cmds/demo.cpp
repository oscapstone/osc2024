#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "interrupt.hpp"
#include "string.hpp"
#include "timer.hpp"

int demo_preempt(int argc, char* argv[]) {
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

int delay_uart(int argc, char* argv[]) {
  mini_uart_delay = argc >= 3 ? strtol(argv[2]) : 10;
  mini_uart_printf("set delay uart %ds\n", mini_uart_delay);
  return 0;
}

int delay_timer(int argc, char* argv[]) {
  timer_delay = argc >= 3 ? strtol(argv[2]) : 10;
  mini_uart_printf("set delay timer %ds\n", timer_delay);
  return 0;
}

int cmd_demo(int argc, char* argv[]) {
  if (argc <= 1) {
    mini_uart_puts("demo: require at least one argument\n");
    return -1;
  }

  auto cmd = argv[1];
  if (!strcmp(cmd, "preempt"))
    return demo_preempt(argc, argv);

  if (!strcmp(cmd, "delay-uart"))
    return delay_uart(argc, argv);

  if (!strcmp(cmd, "delay-timer"))
    return delay_timer(argc, argv);

  mini_uart_printf("demo: task '%s' not found\n", cmd);
  return -1;
}
