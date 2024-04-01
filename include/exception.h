#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#define CORE0_IRQ_SOURCE ((volatile unsigned int *)(0x40000060)) // core 0 interrupt source:https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf

void core_timer_handler();
void uart_interrupt_handler();

void print_current_el(void);
extern void exit_kernel();


extern void move_to_user_mode(void); // defined in exception_.S
extern void enable_interrupt();
extern void disable_interrupt();
extern void enable_irq();
extern void disable_irq();

#endif // __EXCEPTION_H__