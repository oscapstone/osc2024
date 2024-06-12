#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "gpio.h"
#include "timer.h"

// ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf (p. 112)
#define IRQ_PENDING_1 ((volatile unsigned int *)(MMIO_BASE + 0xB204))
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)(0x40000060))

void el1_interrupt_enable();
void el1_interrupt_disable();
void lock();
void unlock();

typedef struct trapframe {
    unsigned long regs[31];
    unsigned long spsr_el1;
    unsigned long elr_el1;
    unsigned long sp_el0;
} trapframe_t;

void exception_handler_c();
void irq_handler_c(trapframe_t *tf);

void highp();
void lowp();
void test_preemption();

void el0_sync_router(trapframe_t *tf);
void el0_irq_router(trapframe_t *tf);
void invalid_op_handler();

#endif