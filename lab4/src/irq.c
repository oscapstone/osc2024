#include "irq.h"

#include "mem.h"
#include "timer.h"
#include "uart.h"

typedef struct __irq_task_t {
  void (*func)();
  int order;
  int busy;  // 0 (default) or 1 (busy)
  struct __irq_task_t *prev;
  struct __irq_task_t *next;
} irq_task_t;

static irq_task_t *head = 0;

// Disable interrupt before calling irq_add_task()
void irq_add_task(void (*callback)(), int order) {
  irq_task_t *task = (irq_task_t *)kmalloc(sizeof(irq_task_t), 1);
  task->func = callback;
  task->order = order;
  task->busy = 0;
  task->prev = 0;
  task->next = 0;

  // 0 -> task -> head -> ...
  if (head == 0 || task->order < head->order) {
    task->next = head;
    task->prev = 0;
    if (head != 0) head->prev = task;
    head = task;
    return;
  }

  irq_task_t *current = head;
  while (current->next != 0 && current->next->order <= task->order)
    current = current->next;
  task->next = current->next;
  if (current->next != 0) current->next->prev = task;
  current->next = task;
  task->prev = current;
}

// to enable interrupts in EL1
void enable_interrupt() {
  asm volatile(
      "msr DAIFClr, 0xF\n"  // Clear the D, A, I, F bits in the DAIF register
  );
}

// to disable interrupts in EL1
void disable_interrupt() {
  asm volatile(
      "msr DAIFSet, 0xF\n"  // Set the D, A, I, F bits to 1 in the DAIF register
  );
}

void irq_entry() {
  disable_interrupt();  // Enter the critical section

  if (*IRQ_PENDING_1 & (1 << 29)) {  // UART interrupt
    switch (*AUX_MU_IIR & 0x6) {     // 0x6 = 0110 -> Get 0x2 and 0x4

      case 0x2:  // 0x2 Transmit interrupt
        disable_uart_tx_interrupt();
        irq_add_task(uart_tx_irq_handler, ORDER_FIRST);
        break;

      case 0x4:  // 0x4 Receive interrupt
        disable_uart_rx_interrupt();
        irq_add_task(uart_rx_irq_handler, ORDER_FIRST);
        break;
    }
  } else if (*CORE0_INTERRUPT_SOURCE & 0x2) {  // Core 0 timer interrupt
    disable_timer_interrupt();
    irq_add_task(timer_irq_handler, ORDER_LAST);
  }

  enable_interrupt();  // Leave the critical section

  // Preemption: run the task with the highest priority
  while (head != 0 && !head->busy) {
    disable_interrupt();
    irq_task_t *task = head;  // Get a task from head
    task->busy = 1;           // Flag the task as under processing
    enable_interrupt();

    task->func();  // Run the tasks with interrupts enabled

    // Remove the task
    disable_interrupt();
    if (task->prev != 0) task->prev->next = task->next;
    if (task->next != 0) task->next->prev = task->prev;
    if (task == head) head = task->next;
    kfree((void *)task, 1);
    enable_interrupt();
  }
}