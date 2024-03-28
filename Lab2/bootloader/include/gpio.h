#ifndef _MMIO_H_
#define _MMIO_H_

#define UI unsigned int
#define UL unsigned long
#define VUI volatile unsigned int

#define MMIO_BASE   0x3F000000

#define GPFSEL0         ((VUI*)(MMIO_BASE+0x00200000))
#define GPFSEL1         ((VUI*)(MMIO_BASE+0x00200004))
#define GPFSEL2         ((VUI*)(MMIO_BASE+0x00200008))
#define GPFSEL3         ((VUI*)(MMIO_BASE+0x0020000C))
#define GPFSEL4         ((VUI*)(MMIO_BASE+0x00200010))
#define GPFSEL5         ((VUI*)(MMIO_BASE+0x00200014))
#define GPSET0          ((VUI*)(MMIO_BASE+0x0020001C))
#define GPSET1          ((VUI*)(MMIO_BASE+0x00200020))
#define GPCLR0          ((VUI*)(MMIO_BASE+0x00200028))
#define GPLEV0          ((VUI*)(MMIO_BASE+0x00200034))
#define GPLEV1          ((VUI*)(MMIO_BASE+0x00200038))
#define GPEDS0          ((VUI*)(MMIO_BASE+0x00200040))
#define GPEDS1          ((VUI*)(MMIO_BASE+0x00200044))
#define GPHEN0          ((VUI*)(MMIO_BASE+0x00200064))
#define GPHEN1          ((VUI*)(MMIO_BASE+0x00200068))
#define GPPUD           ((VUI*)(MMIO_BASE+0x00200094))
#define GPPUDCLK0       ((VUI*)(MMIO_BASE+0x00200098))
#define GPPUDCLK1       ((VUI*)(MMIO_BASE+0x0020009C))

#endif