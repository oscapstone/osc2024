#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "gpio.h"
#include "timer.h"

// ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf (p. 112)
#define IRQ_PENDING_1 ((volatile unsigned int *)(MMIO_BASE + 0xB204))
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)(0x40000060))

void disable_interrupt();
void enable_interrupt();
void exception_handler_c();
void irq_handler_c();

void highp();
void lowp();
void test_preemption();

#endif