#include "int/irq.hpp"

#include "board/mini-uart.hpp"
#include "int/interrupt.hpp"
#include "int/timer.hpp"

void irq_handler(ExceptionContext* context, int type) {
  auto irq_source = get32(CORE0_IRQ_SOURCE);
  if ((irq_source & CNTPNSIRQ_INT) == CNTPNSIRQ_INT)
    timer_enqueue();
  if ((irq_source & GPU_INT) == GPU_INT)
    mini_uart_enqueue();

  irq_run();
}

ListHead<Task> irq_tasks;

void irq_init() {
  irq_tasks.init();
}

void irq_add_task(int prio, Task::fp callback, void* context) {
  auto node = new Task{prio, callback, context};
  auto it = irq_tasks.begin();
  for (; it != irq_tasks.end(); it++)
    if (prio >= it->prio)
      break;
  irq_tasks.insert_before(it, node);
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
