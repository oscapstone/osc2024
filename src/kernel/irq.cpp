#include "irq.hpp"

#include "board/mini-uart.hpp"
#include "interrupt.hpp"
#include "timer.hpp"

void irq_handler(ExceptionContext* context, int type) {
  auto irq_source = get32(CORE0_IRQ_SOURCE);
  if ((irq_source & CNTPNSIRQ_INT) == CNTPNSIRQ_INT)
    timer_enqueue();
  if ((irq_source & GPU_INT) == GPU_INT)
    mini_uart_enqueue();

  irq_run();
}

TaskHead irq_tasks;

void irq_init() {
  irq_tasks.count = 0;
  link(&irq_tasks.head, &irq_tasks.tail);
}

void irq_add_task(int prio, Task::fp callback, void* context) {
  auto node = new Task{prio, callback, context};
  auto it = irq_tasks.begin();
  while (it != irq_tasks.end()) {
    if (prio >= it->prio) {
      break;
    }
    it = it->next;
  }
  irq_tasks.insert(it, node);
}

void irq_run() {
  while (not irq_tasks.empty()) {
    auto task = irq_tasks.begin();
    if (task->running)
      break;
    task->running = true;
    enable_interrupt();
    task->call();
    disable_interrupt();
    irq_tasks.erase(task);
  }
}
