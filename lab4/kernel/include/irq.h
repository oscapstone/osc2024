#ifndef IRQ_H
#define IRQ_H

#include "uart.h"
#include "gpio.h"
#include "timer.h"
#include "task.h"

#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int*)(0x40000060))

#define INTERRUPT_SOURCE_TIMER (1 << 1)
#define INTERRUPT_SOURCE_CNTPNSIRQ 1<<1
#define INTERRUPT_SOURCE_GPU (1 << 8)

#define INT_BASE (MMIO_BASE+0xB000)
#define IRQ_PENDING_1 ((volatile unsigned int*)(INT_BASE+0x204))
#define IRQ_PENDING_1_AUX_INT (1 << 29)



void enable_interrupt();
void disable_interrupt();
void irq_handler(unsigned long long x0);
void invalid_exception_handler(unsigned long long x0);
void cpacr_el1_off();
void sync_el0_64_handler();

void highp();
void lowp();
void test_preemption();

#endif