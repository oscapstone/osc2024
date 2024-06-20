#ifndef	_EXCEPTION_H_
#define	_EXCEPTION_H_

#include "utility.h"
#include "stdint.h"

#define ESR_EL1_EC_SHIFT 26
#define ESR_EL1_EC_MASK 0x3F
#define ESR_EL1_EC_SVC64 0x15


typedef struct irqtask
{
    struct list_head listhead;
    unsigned long long priority; // store priority (smaller number is more preemptive)
    void *task_function;         // task function pointer
} irqtask_t;
void irqtask_list_init();

void irqtask_add(void *task_function, unsigned long long priority);
void irqtask_run(irqtask_t *the_task);
void irqtask_run_preemptive();
void irqtask_list_init();


void lock();
void unlock();
void print_lock();

typedef struct trapframe
{
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
    uint64_t x8;
    uint64_t x9;
    uint64_t x10;
    uint64_t x11;
    uint64_t x12;
    uint64_t x13;
    uint64_t x14;
    uint64_t x15;
    uint64_t x16;
    uint64_t x17;
    uint64_t x18;
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x29;
    uint64_t x30;
    uint64_t spsr_el1;
    uint64_t elr_el1;
    uint64_t sp_el0;
} trapframe_t;
// https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p16
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int*)(0x40000060))

#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
#define INTERRUPT_SOURCE_GPU (1<<8)

#define IRQ_PENDING_1_AUX_INT (1<<29)

// void el1_interrupt_enable();
// void el1_interrupt_disable();

// void el1h_irq_router();
// void el0_sync_router();
// void el0_irq_64_router();

// void invalid_exception_router(); // exception_handler.S

#endif /*_EXCEPTION_H_*/
