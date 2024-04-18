#ifndef GPIO_H
#define GPIO_H

#include "mmio.h"

#define GPIO_BASE       (MMIO_BASE + 0x00200000)

#define GPFSEL0         ((volatile unsigned int*)(GPIO_BASE + 0x00000000))
#define GPFSEL1         ((volatile unsigned int*)(GPIO_BASE + 0x00000004))
#define GPFSEL2         ((volatile unsigned int*)(GPIO_BASE + 0x00000008))
#define GPFSEL3         ((volatile unsigned int*)(GPIO_BASE + 0x0000000C))
#define GPFSEL4         ((volatile unsigned int*)(GPIO_BASE + 0x00000010))
#define GPFSEL5         ((volatile unsigned int*)(GPIO_BASE + 0x00000014))

#define GPSET0          ((volatile unsigned int*)(GPIO_BASE + 0x0000001C))
#define GPSET1          ((volatile unsigned int*)(GPIO_BASE + 0x00000020))

#define GPCLR0          ((volatile unsigned int*)(GPIO_BASE + 0x00000028))
#define GPCLR1          ((volatile unsigned int*)(GPIO_BASE + 0x0000002C))

#define GPLEV0          ((volatile unsigned int*)(GPIO_BASE + 0x00000034))
#define GPLEV1          ((volatile unsigned int*)(GPIO_BASE + 0x00000038))

#define GPEDS0          ((volatile unsigned int*)(GPIO_BASE + 0x00000040))
#define GPEDS1          ((volatile unsigned int*)(GPIO_BASE + 0x00000044))

#define GPHEN0          ((volatile unsigned int*)(GPIO_BASE + 0x00000064))
#define GPHEN1          ((volatile unsigned int*)(GPIO_BASE + 0x00000068))

#define GPLEN0          ((volatile unsigned int*)(GPIO_BASE + 0x00000070))
#define GPLEN1          ((volatile unsigned int*)(GPIO_BASE + 0x00000074))

#define GPAREN0         ((volatile unsigned int*)(GPIO_BASE + 0x0000007C))
#define GPAREN1         ((volatile unsigned int*)(GPIO_BASE + 0x00000080))

#define GPAFEN0         ((volatile unsigned int*)(GPIO_BASE + 0x00000088))
#define GPAFEN1         ((volatile unsigned int*)(GPIO_BASE + 0x0000008C))

#define GPPUD           ((volatile unsigned int*)(GPIO_BASE + 0x00000094))

#define GPPUDCLK0       ((volatile unsigned int*)(GPIO_BASE + 0x00000098))
#define GPPUDCLK1       ((volatile unsigned int*)(GPIO_BASE + 0x0000009C))

#endif