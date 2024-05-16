#ifndef	_EXCEPTION_H_
#define	_EXCEPTION_H_

#include "u_list.h"

#define UART_PRIORITY  1
#define TIMER_PRIORITY 0

typedef struct trapframe
{
    unsigned long x0;
    unsigned long x1;
    unsigned long x2;
    unsigned long x3;
    unsigned long x4;
    unsigned long x5;
    unsigned long x6;
    unsigned long x7;
    unsigned long x8;
    unsigned long x9;
    unsigned long x10;
    unsigned long x11;
    unsigned long x12;
    unsigned long x13;
    unsigned long x14;
    unsigned long x15;
    unsigned long x16;
    unsigned long x17;
    unsigned long x18;
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long x29;
    unsigned long x30;
    unsigned long spsr_el1;
    unsigned long elr_el1;
    unsigned long sp_el0;

} trapframe_t;

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

void lock();
void unlock();

void irq_router(trapframe_t *tp);
void svc_router(trapframe_t *tp);

void invalid_exception_router(unsigned long long x0); // exception_handler.S

#endif /*_EXCEPTION_H_*/
