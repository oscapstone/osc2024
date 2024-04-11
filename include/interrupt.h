#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "peripherals/mmio.h"

#define CORE0_INT_SRC     \
  ((volatile unsigned int \
        *)0x40000060)  // The interrupt source register which
                       // shows what the source bits are for IRQ/FIQ
#define CORE_INT_SRC_TIMER (1 << 1)
#define CORE_INT_SRC_GPU (1 << 8)

#define IRQ_PENDING_1    \
  (volatile unsigned int \
       *)(MMIO_BASE + 0x0000B204)        // IRQ pending source 31:0
                                         // (IRQ table in the BCM2837 document)
#define IRQ_PENDING_1_AUX_INT (1 << 29)  // bit29: AUX INT
#define ENABLE_IRQS_1                    \
  ((volatile unsigned int *)(MMIO_BASE + \
                             0x0000B210))  // Set to enable IRQ source 31:0 (IRQ
                                           // table in the BCM2837 document)

void el0_64_sync_interrupt_handler();
void el0_64_irq_interrupt_handler();
void el1h_irq_interrupt_handler();
void fake_long_handler();
#endif