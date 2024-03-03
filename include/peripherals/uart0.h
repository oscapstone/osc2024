#ifndef _PERIPHERALS_UART0_H
#define _PERIPHERALS_UART0_H

#include "peripherals/mmio.h"

#define UART0_BASE (MMIO_BASE + 0x201000)

#define UART0_DR ((volatile unsigned int *)(UART0_BASE))
#define UART0_FR ((volatile unsigned int *)(UART0_BASE + 0x18))
#define UART0_IBRD ((volatile unsigned int *)(UART0_BASE + 0x24))
#define UART0_FBRD ((volatile unsigned int *)(UART0_BASE + 0x28))
#define UART0_LCRH ((volatile unsigned int *)(UART0_BASE + 0x2C))
#define UART0_CR ((volatile unsigned int *)(UART0_BASE + 0x30))
#define UART0_IMSC ((volatile unsigned int *)(UART0_BASE + 0x38))
#define UART0_ICR ((volatile unsigned int *)(UART0_BASE + 0x44))

#endif