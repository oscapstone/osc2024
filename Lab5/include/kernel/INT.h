#ifndef INT_H
#define INT_H

#include "kernel/gpio.h"
// BCM2837 p.112
#define IRQ_basic_pending       ((volatile unsigned int*)(MMIO_BASE + 0x0000B200))
#define IRQ_pending_1           ((volatile unsigned int*)(MMIO_BASE + 0x0000B204))
#define IRQ_pending_2           ((volatile unsigned int*)(MMIO_BASE + 0x0000B208))
#define FIQ_control             ((volatile unsigned int*)(MMIO_BASE + 0x0000B20C))
#define Enable_IRQs_1           ((volatile unsigned int*)(MMIO_BASE + 0x0000B210))
#define Enable_IRQs_2           ((volatile unsigned int*)(MMIO_BASE + 0x0000B214))
#define Enable_Basic_IRQs       ((volatile unsigned int*)(MMIO_BASE + 0x0000B218))
#define Disable_IRQs_1          ((volatile unsigned int*)(MMIO_BASE + 0x0000B21C))
#define Disable_IRQs_2          ((volatile unsigned int*)(MMIO_BASE + 0x0000B220))
#define Disable_Basic_IRQs      ((volatile unsigned int*)(MMIO_BASE + 0x0000B224))
#define CORE0_INT_SRC           (volatile unsigned int*)(0x40000060)
#define CORE0_TIMER_IRQ_CTRL    (volatile unsigned int*)(0x40000040)

extern int boot_timer_flag;

#endif