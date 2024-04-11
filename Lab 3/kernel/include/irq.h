#ifndef __IRQ_H__
#define __IRQ_H__

#include "mmio.h"

#define IRQ_BASIC               ((volatile uint32ptr_t) (MMIO_BASE + 0xb200))
#define IRQ_PENDING1            ((volatile uint32ptr_t) (MMIO_BASE + 0xb204))
#define IRQ_ENABLE1             ((volatile uint32ptr_t) (MMIO_BASE + 0xb210))
#define IRQ_DISABLE1            ((volatile uint32ptr_t) (MMIO_BASE + 0xb21c))


#define irq_enable_aux_interrupt() { *IRQ_ENABLE1 = (1 << 29); }
#define irq_disable_aux_interrupt() { *IRQ_DISABLE1 = (1 << 29); }

// void irq_enable_aux_interrupt()
// {
//     *IRQ_ENABLE1 = (1 << 29);
// } 

// void irq_disable_aux_interrupt()
// {
//     *IRQ_DISABLE1 = (1 << 29);
// }


#endif