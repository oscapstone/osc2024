#include "irq.h"

#include "malloc.h"
#include "timer.h"
#include "uart.h"

typedef struct __irq_task_t {
  void (*func)();
  int priority;
  int busy;  // 0 (default) or 1 (busy)
  struct __irq_task_t *prev;
  struct __irq_task_t *next;
} irq_task_t;

static irq_task_t *head = 0;

/* Disable interrupt before calling irq_add_task() */
void irq_add_task(void (*callback)(), int priority) {
  irq_task_t *task = (irq_task_t *)simple_malloc(sizeof(irq_task_t));
  task->func = callback;
  task->priority = priority;
  task->busy = 0;
  task->prev = 0;
  task->next = 0;

  if (head == 0 || task->priority < head->priority) {
    task->next = head;
    task->prev = 0;
    if (head != 0) head->prev = task;
    head = task;
    return;
  }

  irq_task_t *current = head;
  while (current->next != 0 && current->next->priority <= task->priority)
    current = current->next;
  task->next = current->next;
  if (current->next != 0) current->next->prev = task;
  current->next = task;
  task->prev = current;
}

void enable_interrupt() {
  // to enable interrupts in EL1
  asm volatile(  // Clear the D, A, I, F bits in the DAIF register
      "msr DAIFClr, 0xF;");
}

void disable_interrupt() {
  // to disable interrupts in EL1
  asm volatile(  // Set the D, A, I, F bits to 1 in the DAIF register
      "msr DAIFSet, 0xF;");
}

void irq_entry() {
  disable_interrupt();  // Enter the critical section

  if (*IRQ_PENDING_1 & (1 << 29)) {  // UART interrupt
    // 0110 -> 0x2: Transmit interrupt, 0x4: Receive interrupt
    switch (*AUX_MU_IIR & 0x6) {
      case 0x2:
        uart_disable_tx_interrupt();
        irq_add_task(uart_tx_irq_handler, PRIORITY_HIGH);
        break;
      case 0x4:
        uart_disable_rx_interrupt();
        irq_add_task(uart_rx_irq_handler, PRIORITY_HIGH);
        break;
    }
  } else if (*CORE0_INTERRUPT_SOURCE & 0x2) {  // Core 0 timer interrupt
    timer_disable_interrupt();
    irq_add_task(timer_irq_handler, PRIORITY_LOW);
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
    enable_interrupt();
  }
}