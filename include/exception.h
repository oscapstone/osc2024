#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include "mm.h"

#define CORE0_IRQ_SOURCE ((volatile unsigned int *)(0x40000060 | KERNEL_VADDR_BASE)) // core 0 interrupt source:https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf
// #define ENABLE_TASKLET

void core_timer_handler();
void uart_interrupt_handler();

void print_current_el(void);
/* load context from sp_el1 and eret. It may return to el0 or el1, depending what's stored in spsr_el1. */
extern void exit_kernel();

/* Move to user mode and enable interrupt in el0. Defined in exception_.S */
extern void move_to_user_mode(void);
/* Enable interrupt. (DAIF) */
extern void enable_interrupt();
/* Disable interrupt. (DAIF) */
extern void disable_interrupt();
/* Enable IRQ only. */
extern void enable_irq();
/* Disable IRQ only. */
extern void disable_irq();

#endif // __EXCEPTION_H__