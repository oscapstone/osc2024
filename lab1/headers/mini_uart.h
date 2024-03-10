#ifndef UART_H
#define UART_H

#include "gpio.h"

#define AUX_OFFSET BUS2PHY(0x7E210000)

#define AUX_IRQ             (( volatile unsigned int *)(AUX_OFFSET + 0x5000))
#define AUX_ENABLES         (( volatile unsigned int *)(AUX_OFFSET + 0x5004))

#define AUX_MU_IO_REG       (( volatile unsigned int *)(AUX_OFFSET + 0x5040))
#define AUX_MU_IER_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5044))
#define AUX_MU_IIR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5048))
#define AUX_MU_LCR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x504C))
#define AUX_MU_MCR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5050))
#define AUX_MU_LSR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5054))
#define AUX_MU_MSR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5058))
#define AUX_MU_SCRATCH      (( volatile unsigned int *)(AUX_OFFSET + 0x505C))
#define AUX_MU_CNTL_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5060))
#define AUX_MU_STAT_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5064))
#define AUX_MU_BAUD_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5068))

#define AUX_SPI1_CNTL0_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x5080))
#define AUX_SPI1_CNTL1_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x5084))
#define AUX_SPI1_STAT_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x5088))

#define AUX_SPI1_IO_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5090))
#define AUX_SPI1_PEEK_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x5094))

#define AUX_SPI2_CNTL0_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x50C0))
#define AUX_SPI2_CNTL1_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x50C4))
#define AUX_SPI2_STAT_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x50C8))

#define AUX_SPI2_IO_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x50D0))
#define AUX_SPI2_PEEK_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x50D4))

void mini_uart_init();
char mini_uart_getc();
void mini_uart_putc( char c);
void mini_uart_puts( char *s);

#endif