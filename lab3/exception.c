#include "include/exception.h"
#include "include/irq.h"
#include "include/shell.h"
#include "include/timer.h"
#include "include/types.h"
#include "include/uart.h"

extern irq_task_min_heap_t *irq_task_heap;
extern double_linked_node_t *timer_list_head;
extern int current_irq_task_priority;

void el1h_irq_router() {
  if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT &&
      *CORE0_IRQ_SOURCE & INTERRUPT_SOURCE_GPU) {
    if (*AUX_MU_IIR & 0x04) { // Check if it's a receive interrupt
      *AUX_MU_IER &= ~0x01;
      irq_task_min_heap_push(
          irq_task_heap,
          create_irq_task(uart_rx_handler, NULL, UART_IRQ_PRIORITY));
      irq_task_run_preemptive();
    } else if (*AUX_MU_IIR & 0x02) { // Check if it's a transmit interrupt
      *AUX_MU_IER &= ~0x02;
      irq_task_min_heap_push(
          irq_task_heap,
          create_irq_task(uart_tx_handler, NULL, UART_IRQ_PRIORITY));
      irq_task_run_preemptive();
    }
  }
  if (*CORE0_IRQ_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) {
    core_timer_disable();
    core_timer_handler();
    irq_task_run_preemptive();
    core_timer_enable();
  }
};

void el0_sync_router() {
  unsigned long spsr_el1, elr_el1, esr_el1;
  asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
  asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
  asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
  uart_sendline("[EL0 Synchronous Exception Occurred]\n");
  uart_sendline("spsr_el1 : 0x%x, elr_el1 : 0x%x, esr_el1 : 0x%x\n", spsr_el1,
                elr_el1, esr_el1);
};

void el0_irq_64_router() {
  if (*CORE0_IRQ_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) {
    core_timer_disable();
    timer_task_t *task = (timer_task_t *)timer_list_head->next;
    double_linked_remove(timer_list_head->next);
    core_timer_update();
    core_timer_enable();
    ((void (*)(char *))task->callback)(task->callback_arg);
  }
};

void invalid_exception_router(){
    // todo
};

void irq_task_run_preemptive() {
  while (irq_task_heap->size > 0) {
    el1_interrupt_disable();
    irq_task_t *task = irq_task_min_heap_get_min(irq_task_heap);
    if (task == NULL) {
      el1_interrupt_enable();
      return;
    }
    if (current_irq_task_priority > task->priority) {
      irq_task_min_heap_pop(irq_task_heap);
      int prev_task_priority = current_irq_task_priority;
      current_irq_task_priority = task->priority;
      el1_interrupt_enable();

      ((void (*)(char *))task->callback)(task->callback_arg);

      el1_interrupt_disable();
      current_irq_task_priority = prev_task_priority;
      el1_interrupt_enable();

    } else {
      el1_interrupt_enable();
      return;
    }
  }
}