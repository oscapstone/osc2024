#ifndef _IRQ_H
#define _IRQ_H
#include "peripherals/base.h"

#define IRQ_BASIC_PENDING	((volatile unsigned int *)(PBASE+0x0000B200))
#define IRQ_PENDING_1		((volatile unsigned int *)(PBASE+0x0000B204))
#define IRQ_PENDING_2		((volatile unsigned int *)(PBASE+0x0000B208))
#define FIQ_CONTROL		    ((volatile unsigned int *)(PBASE+0x0000B20C))
#define ENABLE_IRQS_1		((volatile unsigned int *)(PBASE+0x0000B210))
#define ENABLE_IRQS_2		((volatile unsigned int *)(PBASE+0x0000B214))
#define ENABLE_BASIC_IRQS	((volatile unsigned int *)(PBASE+0x0000B218))
#define DISABLE_IRQS_1		((volatile unsigned int *)(PBASE+0x0000B21C))
#define DISABLE_IRQS_2		((volatile unsigned int *)(PBASE+0x0000B220))
#define DISABLE_BASIC_IRQS	((volatile unsigned int *)(PBASE+0x0000B224))


#endif // _IRQ_H