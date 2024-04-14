#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "int/interrupt.hpp"
#include "int/timer.hpp"
#include "string.hpp"

bool show_timer = false;
int timer_delay = 0;

void print_2s_timer(void* context) {
  if (timer_delay > 0) {
    auto delay = timer_delay;
    timer_delay = 0;
    mini_uart_printf_sync("delay timer %ds\n", delay);
    auto cur = get_current_tick();
    while (get_current_tick() - cur < delay * freq_of_timer)
      NOP;
  }
  if (show_timer) {
    mini_uart_printf("[" PRTval "] 2s timer interrupt\n",
                     FTval(get_current_time()));
    add_timer({2, 0}, (void*)context, (Timer::fp)context);
  }
}

int demo_timer(int argc, char* argv[]) {
  if (show_timer) {
    show_timer = false;
  } else {
    show_timer = true;
    add_timer({2, 0}, (void*)print_2s_timer, print_2s_timer);
  }
  mini_uart_printf("%s timer interrupt\n", show_timer ? "show" : "hide");
  return 0;
}

int demo_preempt(int argc, char* argv[]) {
  auto print_data = [](void* ctx) {
    mini_uart_printf("print from task %ld\n", (long)ctx);
  };

  save_DAIF_disable_interrupt();

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

  if (!strcmp(cmd, "timer"))
    return demo_timer(argc, argv);

  if (!strcmp(cmd, "preempt"))
    return demo_preempt(argc, argv);

  if (!strcmp(cmd, "delay-uart"))
    return delay_uart(argc, argv);

  if (!strcmp(cmd, "delay-timer"))
    return delay_timer(argc, argv);

  mini_uart_printf("demo: task '%s' not found\n", cmd);
  return -1;
}
