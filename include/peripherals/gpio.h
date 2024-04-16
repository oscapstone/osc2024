#ifndef _GPIO_H
#define _GPIO_H

#include "peripherals/mmio.h"
#include "types.h"

// Base address for GPIO peripherals
#define GPIO_BASE (MMIO_BASE + 0x200000)

// GPIO Function Select registers
#define GPFSEL0 ((volatile uint32_t *)(GPIO_BASE + 0x00))
#define GPFSEL1 ((volatile uint32_t *)(GPIO_BASE + 0x04))
#define GPFSEL2 ((volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPFSEL3 ((volatile uint32_t *)(GPIO_BASE + 0x0C))
#define GPFSEL4 ((volatile uint32_t *)(GPIO_BASE + 0x10))
#define GPFSEL5 ((volatile uint32_t *)(GPIO_BASE + 0x14))
// 0x18 Reserved

// GPIO Pin Output Set registers
#define GPSET0 ((volatile uint32_t *)(GPIO_BASE + 0x1C))
#define GPSET1 ((volatile uint32_t *)(GPIO_BASE + 0x20))
// 0x24 Reserved

// GPIO Pin Output Clear registers
#define GPCLR0 ((volatile uint32_t *)(GPIO_BASE + 0x28))
#define GPCLR1 ((volatile uint32_t *)(GPIO_BASE + 0x2C))
// 0x30 Reserved

// GPIO Pin Level registers
#define GPLEV0 ((volatile uint32_t *)(GPIO_BASE + 0x34))
#define GPLEV1 ((volatile uint32_t *)(GPIO_BASE + 0x38))
// 0x3C Reserved

// GPIO Pin Event Detect Status registers
#define GPEDS0 ((volatile uint32_t *)(GPIO_BASE + 0x40))
#define GPEDS1 ((volatile uint32_t *)(GPIO_BASE + 0x44))
// 0x48 Reserved

// GPIO Pin Rising Edge Detect Enable registers
#define GPREN0 ((volatile uint32_t *)(GPIO_BASE + 0x4C))
#define GPREN1 ((volatile uint32_t *)(GPIO_BASE + 0x50))
// 0x54 Reserved

// GPIO Pin Falling Edge Detect Enable registers
#define GPFEN0 ((volatile uint32_t *)(GPIO_BASE + 0x58))
#define GPFEN1 ((volatile uint32_t *)(GPIO_BASE + 0x5C))
// 0x60 Reserved

// GPIO Pin High Level Detect Enable registers
#define GPHEN0 ((volatile uint32_t *)(GPIO_BASE + 0x64))
#define GPHEN1 ((volatile uint32_t *)(GPIO_BASE + 0x68))
// 0x6C Reserved

// GPIO Pin Low Level Detect Enable registers
#define GPLEN0 ((volatile uint32_t *)(GPIO_BASE + 0x70))
#define GPLEN1 ((volatile uint32_t *)(GPIO_BASE + 0x74))
// 0x78 Reserved

// GPIO Pin Asynchronous Rising Edge Detect Enable registers
#define GPAREN0 ((volatile uint32_t *)(GPIO_BASE + 0x7C))
#define GPAREN1 ((volatile uint32_t *)(GPIO_BASE + 0x80))
// 0x84 Reserved

// GPIO Pin Asynchronous Falling Edge Detect Enable registers
#define GPAFEN0 ((volatile uint32_t *)(GPIO_BASE + 0x88))
#define GPAFEN1 ((volatile uint32_t *)(GPIO_BASE + 0x8C))
// 0x90 Reserved

// GPIO Pin Pull-up/down Enable register
#define GPPUD ((volatile uint32_t *)(GPIO_BASE + 0x94))

// GPIO Pin Pull-up/down Enable Clock registers
#define GPPUDCLK0 ((volatile uint32_t *)(GPIO_BASE + 0x98))
#define GPPUDCLK1 ((volatile uint32_t *)(GPIO_BASE + 0x9C))

#endif