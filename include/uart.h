/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef UART_H
#define UART_H

#include "gpio.h"

// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

#define IRQ_BASIC       ((volatile unsigned int*)(IRQ_BASE+0x00000200)) // irq basic pending
#define IRQ_PEND1       ((volatile unsigned int*)(IRQ_BASE+0x00000204)) // irq pending 1
#define IRQ_PEND2       ((volatile unsigned int*)(IRQ_BASE+0x00000208)) // irq pending 2
#define IRQ_FIQ         ((volatile unsigned int*)(IRQ_BASE+0x0000020C)) // irq FIQ control
#define IRQ_ENABLE1     ((volatile unsigned int*)(IRQ_BASE+0x00000210)) // irq enable 1
#define IRQ_ENABLE2     ((volatile unsigned int*)(IRQ_BASE+0x00000214)) // irq enable 2
#define IRQ_ENABLE_BASIC ((volatile unsigned int*)(IRQ_BASE+0x00000218)) // irq enable basic
#define IRQ_DISABLE1    ((volatile unsigned int*)(IRQ_BASE+0x0000021C)) // irq disable 1
#define IRQ_DISABLE2    ((volatile unsigned int*)(IRQ_BASE+0x00000220)) // irq disable 2
#define IRQ_DISABLE_BASIC ((volatile unsigned int*)(IRQ_BASE+0x00000224)) // irq disable basic

#define IRQS1 (volatile unsigned int *)0x3f00b210

void uart_init();
void uart_async_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(unsigned int d);

void printf(char *fmt, ...);
void uart_dump(void *ptr);

#endif
