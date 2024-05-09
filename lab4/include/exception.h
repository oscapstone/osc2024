#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "heap.h"
#include "types.h"
#include "uart.h"

#define CORE0_IRQ_SOURCE ((volatile unsigned int *)(0x40000060))
#define INTERRUPT_SOURCE_CNTPNSIRQ (1 << 1)
#define INTERRUPT_SOURCE_GPU (1 << 8)

#define IRQ_HEAP_CAPACITY 100
#define UART_IRQ_PRIORITY 2
#define TIMER_IRQ_DEFAULT_PRIORITY 0

typedef struct irq_task {
  void *callback;
  void *callback_arg;
  int priority;
} irq_task_t;

typedef struct irq_task_min_heap {
  irq_task_t **tasks;
  int size;
  int capacity;
} irq_task_min_heap_t;

static inline void el1_interrupt_enable() {
  __asm__ __volatile__("msr daifclr, 0xf");
}

static inline void el1_interrupt_disable() {
  __asm__ __volatile__("msr daifset, 0xf");
}

static inline void irq_task_min_heap_init(irq_task_min_heap_t *heap,
                                          int capacity) {
  heap->tasks =
      (irq_task_t **)simple_malloc(sizeof(irq_task_t *) * capacity, 0);
  if (heap->tasks == NULL) {
    uart_sendline("Error: Failed to allocate memory for irq task min heap.\n");
    return;
  }
  heap->size = 0;
  heap->capacity = capacity;
}

static inline void irq_task_min_heap_push(irq_task_min_heap_t *heap,
                                          irq_task_t *task) {
  if (task == NULL) {
    uart_sendline("Error: Cannot add a NULL irq task.\n");
    return;
  }
  el1_interrupt_disable();
  if (heap->size == heap->capacity) {
    el1_interrupt_enable();
    return;
  }
  heap->tasks[heap->size] = task;
  int current = heap->size;
  heap->size++;
  while (current != 0 && heap->tasks[current]->priority <
                             heap->tasks[(current - 1) / 2]->priority) {
    irq_task_t *temp = heap->tasks[current];
    heap->tasks[current] = heap->tasks[(current - 1) / 2];
    heap->tasks[(current - 1) / 2] = temp;
    current = (current - 1) / 2;
  }
  el1_interrupt_enable();
}

static inline void irq_task_min_heap_pop(irq_task_min_heap_t *heap) {
  if (heap->size == 0) {
    return;
  }
  heap->size--;
  heap->tasks[0] = heap->tasks[heap->size];
  int current = 0;
  while (2 * current + 1 < heap->size) {
    int child = 2 * current + 1;
    if (child + 1 < heap->size &&
        heap->tasks[child + 1]->priority < heap->tasks[child]->priority) {
      child++;
    }
    if (heap->tasks[current]->priority <= heap->tasks[child]->priority) {
      break;
    }
    irq_task_t *temp = heap->tasks[current];
    heap->tasks[current] = heap->tasks[child];
    heap->tasks[child] = temp;
    current = child;
  }
  return;
}

static inline irq_task_t *create_irq_task(void *callback, char *arg,
                                          int priority) {
  irq_task_t *task = simple_malloc(sizeof(irq_task_t), 0);
  if (task == NULL) {
    uart_sendline("Error: Failed to allocate memory for the irq task.\n");
    return NULL;
  }
  task->callback = callback;
  task->callback_arg = arg;
  task->priority = priority;
  return task;
}

static inline irq_task_t *
irq_task_min_heap_get_min(const irq_task_min_heap_t *heap) {
  if (heap->size == 0) {
    return NULL;
  }
  return heap->tasks[0];
}

void el1h_irq_router();
void el0_sync_router();
void el0_irq_64_router();
void invalid_exception_router();
void irq_task_run_preemptive();

#endif /* EXCEPTION_H */