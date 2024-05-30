#include "board/mini-uart.hpp"
#include "ds/timeval.hpp"
#include "int/interrupt.hpp"
#include "int/timer.hpp"
#include "io.hpp"
#include "mm/mm.hpp"
#include "mm/page.hpp"
#include "sched.hpp"
#include "shell/cmd.hpp"
#include "thread.hpp"
#include "util.hpp"

bool show_timer = false;
int timer_delay = 0;

void print_2s_timer(void* context) {
  if (timer_delay > 0) {
    auto delay = timer_delay;
    timer_delay = 0;
    kprintf("delay timer %ds\n", delay);
    auto cur = get_current_tick();
    while (get_current_tick() - cur < delay * freq_of_timer)
      NOP;
  }
  if (show_timer) {
    kprintf("[" PRTval "] 2s timer interrupt\n", FTval(get_current_time()));
    add_timer({2, 0}, (void*)context, (Timer::fp)context);
  }
}

int demo_timer(int /*argc*/, char* /*argv*/[]) {
  if (show_timer) {
    show_timer = false;
  } else {
    show_timer = true;
    add_timer({2, 0}, (void*)print_2s_timer, print_2s_timer);
  }
  kprintf("%s timer interrupt\n", show_timer ? "show" : "hide");
  return 0;
}

int demo_preempt(int /*argc*/, char* /*argv*/[]) {
  auto print_data = [](void* ctx) {
    kprintf("print from task %ld\n", (long)ctx);
  };

  save_DAIF_disable_interrupt();

  add_timer(freq_of_timer, (void*)1, print_data, 1);
  add_timer(freq_of_timer, (void*)0, print_data, 0);
  add_timer(freq_of_timer, (void*)2, print_data, 2);

  restore_DAIF();

  return 0;
}

int delay_uart(int argc, char* argv[]) {
  mini_uart_delay = argc >= 2 ? strtol(argv[1]) : 10;
  kprintf("set delay uart %ds\n", mini_uart_delay);
  return 0;
}

int delay_timer(int argc, char* argv[]) {
  timer_delay = argc >= 2 ? strtol(argv[1]) : 10;
  kprintf("set delay timer %ds\n", timer_delay);
  return 0;
}

int demo_page(int argc, char* argv[]) {
  auto size = argc >= 2 ? strtol(argv[1]) : PAGE_SIZE * 8;
  kprintf("demo page: size = 0x%lx\n", size);
  mm_page.info();
  auto a = mm_page.alloc(size);
  auto b = mm_page.alloc(size);
  auto c = mm_page.alloc(size);
  auto d = mm_page.alloc(size);
  mm_page.info();
  mm_page.free(c);
  mm_page.free(b);
  mm_page.free(d);
  mm_page.free(a);
  mm_page.info();
  return 0;
}

int demo_thread(int /*argc*/, char* /*argv*/[]) {
  auto foo = +[](void*) {
    for (int i = 0; i < 10; ++i) {
      kprintf("Thread id: %d %d\n", current_thread()->tid, i);
      delay(1000000);
      schedule();
    }
  };
  for (int i = 0; i < 2; ++i)
    kthread_create(foo);
  return 0;
}

int cmd_demo(int argc, char* argv[]) {
  if (argc <= 1) {
    kprintf("%s: require at least one argument\n", argv[0]);
    return -1;
  }

  auto cmd = argv[1];

  if (!strcmp(cmd, "timer"))
    return demo_timer(argc - 1, argv + 1);

  if (!strcmp(cmd, "preempt"))
    return demo_preempt(argc - 1, argv + 1);

  if (!strcmp(cmd, "delay-uart"))
    return delay_uart(argc - 1, argv + 1);

  if (!strcmp(cmd, "delay-timer"))
    return delay_timer(argc - 1, argv + 1);

  if (!strcmp(cmd, "page"))
    return demo_page(argc - 1, argv + 1);

  if (!strcmp(cmd, "thread"))
    return demo_thread(argc - 1, argv + 1);

  kprintf("demo: task '%s' not found\n", cmd);
  return -1;
}
