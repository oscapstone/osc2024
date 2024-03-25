#ifndef GPIO_H
#define GPIO_H

// peripheral offset of the GPIO and the UART hardware systems, as well as some of their registers.

// The GPIO registers base address.
// Check BCM2835 section 1.2.3 
// Check p.90 of BCM2837 doc
#define MMIO_BASE   0x3F000000 // 0x3F200000 for raspi2 & 3, 0x20200000 for raspi1
// GPIO Function Select n
#define GPFSEL0     ((volatile unsigned int*)(MMIO_BASE + 0x00200000))
#define GPFSEL1     ((volatile unsigned int*)(MMIO_BASE + 0x00200004))
#define GPFSEL2     ((volatile unsigned int*)(MMIO_BASE + 0x00200008))
#define GPFSEL3     ((volatile unsigned int*)(MMIO_BASE + 0x0020000C))
#define GPFSEL4     ((volatile unsigned int*)(MMIO_BASE + 0x00200010))
#define GPFSEL5     ((volatile unsigned int*)(MMIO_BASE + 0x00200014))
// GPIO Pin Output Set n
#define GPSET0      ((volatile unsigned int*)(MMIO_BASE + 0x0020001C))
#define GPSET1      ((volatile unsigned int*)(MMIO_BASE + 0x00200020))
// GPIO Pin Output Clear n
#define GPCLR0      ((volatile unsigned int*)(MMIO_BASE + 0x00200028))
#define GPCLR1      ((volatile unsigned int*)(MMIO_BASE + 0x0020002C))
// GPIO Pin Level n
#define GPLEV0      ((volatile unsigned int*)(MMIO_BASE + 0x00200034))
#define GPLEV1      ((volatile unsigned int*)(MMIO_BASE + 0x00200038))
// GPIO Pin Event Detect Status n
#define GPEDS0      ((volatile unsigned int*)(MMIO_BASE + 0x00200040))
#define GPEDS1      ((volatile unsigned int*)(MMIO_BASE + 0x00200044))
// GPIO Pin Rising Edge Detect Enable n
#define GPREN0      ((volatile unsigned int*)(MMIO_BASE + 0x0020004C))
#define GPREN1      ((volatile unsigned int*)(MMIO_BASE + 0x00200050))
// GPIO Pin Falling Edge Detect Enable n
#define GPFEN0      ((volatile unsigned int*)(MMIO_BASE + 0x00200058))
#define GPFEN1      ((volatile unsigned int*)(MMIO_BASE + 0x0020005C))
// GPIO Pin High Detect Enable n
#define GPHEN0      ((volatile unsigned int*)(MMIO_BASE + 0x00200064))
#define GPHEN1      ((volatile unsigned int*)(MMIO_BASE + 0x00200068))
// GPIO Pin Low Detect Enable
#define GPLEN0      ((volatile unsigned int*)(MMIO_BASE + 0x00200070))
#define GPLEN1      ((volatile unsigned int*)(MMIO_BASE + 0x00200074))
// GPIO Pin Async. Rising Edge Detect n
#define GPAREN0     ((volatile unsigned int*)(MMIO_BASE + 0x0020007C))
#define GPAREN1     ((volatile unsigned int*)(MMIO_BASE + 0x00200080))
// GPIO Pin Async. Falling Edge Detect n
#define GPAFEN0     ((volatile unsigned int*)(MMIO_BASE + 0x00200088))
#define GPAFEN1     ((volatile unsigned int*)(MMIO_BASE + 0x0020008C))
// GPIO Pin Pull-up/down Enable
#define GPPUD       ((volatile unsigned int*)(MMIO_BASE + 0x00200094))
// GPIO Pin Pull-up/down Enable Clock n
#define GPPUDCLK0   ((volatile unsigned int*)(MMIO_BASE + 0x00200098))
#define GPPUDCLK1   ((volatile unsigned int*)(MMIO_BASE + 0x0020009C))

void mmio_write(long reg, unsigned int data);

unsigned int mmio_read(long reg);

#endif