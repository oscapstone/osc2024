#pragma once

#include "hardware.h"
#include "traps.h"

#define ORDER_FIRST 0x0
#define ORDER_REGULAR 0x8
#define ORDER_LAST 0xF

typedef struct irq_task_t {
  void (*func)();
  int order;
  int busy;  // 0 (default) or 1 (busy)
  struct irq_task_t *prev;
  struct irq_task_t *next;
} irqTask;

void enable_interrupt();
void disable_interrupt();
void irq_entry(trap_frame *tf);