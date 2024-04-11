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

ListHead<Task> irq_tasks;

void irq_add_task(int prio, Task::fp callback, void* context) {
  // TODO: order by prio
  auto node = new Task{prio, callback, context};
  irq_tasks.insert(irq_tasks.begin(), node);
}

void irq_run() {
  while (not irq_tasks.empty()) {
    auto head = irq_tasks.begin();
    irq_tasks.erase(head);
    enable_interrupt();
    head->call();
    disable_interrupt();
  }
}
