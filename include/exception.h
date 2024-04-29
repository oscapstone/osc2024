#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#define CORE0_IRQ_SOURCE ((volatile unsigned int *)(0x40000060)) // core 0 interrupt source:https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf

void core_timer_handler();
void uart_interrupt_handler();

void print_current_el(void);
/* load_all register from stack and eret. */
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