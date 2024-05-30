#ifndef _RPI_IRQ_H_
#define _RPI_IRQ_H_

#include "peripherals/base.h"

/*
The basic pending register shows which interrupt are pending. To speed up interrupts processing, a
number of 'normal' interrupt status bits have been added to this register. This makes the 'IRQ
pending base' register different from the other 'base' interrupt registers
p112-115 https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
*/

#define IRQ_BASIC_PENDING	((volatile unsigned int*)(PBASE+0x0000B200))
#define IRQ_PENDING_1		((volatile unsigned int*)(PBASE+0x0000B204))
#define IRQ_PENDING_2		((volatile unsigned int*)(PBASE+0x0000B208))
#define FIQ_CONTROL		    ((volatile unsigned int*)(PBASE+0x0000B20C))
#define ENABLE_IRQS_1		((volatile unsigned int*)(PBASE+0x0000B210))
#define ENABLE_IRQS_2		((volatile unsigned int*)(PBASE+0x0000B214))
#define ENABLE_BASIC_IRQS	((volatile unsigned int*)(PBASE+0x0000B218))
#define DISABLE_IRQS_1		((volatile unsigned int*)(PBASE+0x0000B21C))
#define DISABLE_IRQS_2		((volatile unsigned int*)(PBASE+0x0000B220))
#define DISABLE_BASIC_IRQS	((volatile unsigned int*)(PBASE+0x0000B224))

#endif /*_RPI_IRQ_H_*/
