#ifndef _UART_H_
#define _UART_H_

#include "gpio.h"

/* UART */

#define AUX_IRQ         ((VUI*)(MMIO_BASE + 0x00215000))
#define AUX_ENABLES     ((VUI*)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO_REG   ((VUI*)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER_REG  ((VUI*)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR_REG  ((VUI*)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR_REG  ((VUI*)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR_REG  ((VUI*)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR_REG  ((VUI*)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR_REG  ((VUI*)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH  ((VUI*)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL_REG ((VUI*)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT_REG ((VUI*)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD_REG ((VUI*)(MMIO_BASE + 0x00215068))

void uart_init();
char uart_getc();
int  uart_getint();
void uart_send(UI c);
void uart_puts(char* str);

/* Display a binary value in hexadecimal */
void uart_2hex(UI d);

#endif