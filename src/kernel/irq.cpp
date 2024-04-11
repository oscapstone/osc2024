#include "irq.hpp"

#include "board/mini-uart.hpp"
#include "timer.hpp"

void irq_handler(ExceptionContext* context, int type) {
  auto irq_source = get32(CORE0_IRQ_SOURCE);
  if ((irq_source & CNTPNSIRQ_INT) == CNTPNSIRQ_INT)
    timer_enqueue();
  if ((irq_source & GPU_INT) == GPU_INT)
    mini_uart_enqueue();

  irq_run();
}

ListHead<Task> irq_tasks;

void irq_add_task(int prio, Task::fp callback, void* context) {
  // TODO: order by prio
  auto node = new Task{prio, callback, context};
  irq_tasks.insert(irq_tasks.begin(), node);
}

void irq_run() {
  if (irq_tasks.empty())
    return;
  auto head = irq_tasks.begin();
  irq_tasks.erase(head);
  head->call();
}
