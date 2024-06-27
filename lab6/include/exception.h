#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "bcm2837/rpi_irq.h"

#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)(PHYS_TO_VIRT(0x40000060)))

#define IRQ_PENDING_1_AUX_INT (1 << 29)
#define INTERRUPT_SOURCE_GPU (1 << 8)
#define INTERRUPT_SOURCE_CNTPNSIRQ (1 << 1)

typedef struct trapframe {
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

void sync_64_router(trapframe_t *tpf);
void irq_router(trapframe_t *tpf);

static inline void el1_interrupt_enable() { __asm__ __volatile__("msr daifclr, 0xf"); }

static inline void el1_interrupt_disable() { __asm__ __volatile__("msr daifset, 0xf"); }

void lock();
void unlock();

#endif /* _EXCEPTION_H_ */
