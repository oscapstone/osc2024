#ifndef	_EXCEPTION_H_
#define	_EXCEPTION_H_

#include "u_list.h"

#define UART_PRIORITY  1
#define TIMER_PRIORITY 0

typedef struct irqtask
{
    struct list_head listhead;
    unsigned long long priority; // store priority (smaller number is more preemptive)
    void *callback;         // task function pointer
} irqtask_t;

void task_list_init();
void add_task_list(void *callback, unsigned long long priority);
void run_task(irqtask_t *the_task);
void preemption();

// https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p16
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int*)(0x40000060))

#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
#define INTERRUPT_SOURCE_GPU (1<<8)

#define IRQ_PENDING_1_AUX_INT (1<<29)

void enable_irq();
void disable_irq();

void irq_router();
void svc_router();

void invalid_exception_router(unsigned long long x0); // exception_handler.S

#endif /*_EXCEPTION_H_*/
