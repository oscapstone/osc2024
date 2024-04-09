#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#define MMIO_BASE 0x3F000000

#define IRQ_PENDING_1           ((volatile unsigned int*)(MMIO_BASE + 0xB204))
#define ENABLE_IRQS_1           ((volatile unsigned int*)(MMIO_BASE + 0xB210))
#define DISABLE_IRQS_1          ((volatile unsigned int*)(MMIO_BASE + 0xB21C))

#define CORE0_TIMER_IRQ_CTRL    ((volatile unsigned int*)(0x40000040))
#define CORE0_IRQ_SOURCE        ((volatile unsigned int*)(0x40000060))

#define IRQ_SOURCE_CNTPNSIRQ    (1 << 1)
#define IRQ_SOURCE_GPU          (1 << 8)
#define IRQ_PENDING_1_AUX_INT   (1 << 29) //29 bit is Aux int function


void exception_entry();
void disable_interrupt();
void el0_irq_entry();

#endif