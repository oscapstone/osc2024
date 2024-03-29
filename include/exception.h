#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#define CORE0_IRQ_SOURCE ((volatile unsigned int*)(0x40000060))


void core_timer_handler();
void uart_interrupt_handler();

void print_current_el(void);
extern void exit_kernel();

static inline void enable_interrupt()
{
    asm volatile("msr daifclr, 0xf  \n\t");
}

static inline void disable_interrupt()
{
    asm volatile("msr daifset, 0xf  \n\t");
}

#endif // __EXCEPTION_H__