#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#define CORE0_IRQ_SOURCE ((volatile unsigned int*)(0x40000060))


void core_timer_handler();
void uart_interrupt_handler();


#endif // __EXCEPTION_H__