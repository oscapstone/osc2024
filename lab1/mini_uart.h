#ifndef UART_H
#define UART_H

#include "gpio.h"

#define AUX_IRQ             (( volatile unsigned int *)0x7E215000)
#define AUX_ENABLES         (( volatile unsigned int *)0x7E215004)

#define AUX_MU_IO_REG       (( volatile unsigned int *)0x7E215040)
#define AUX_MU_IER_REG      (( volatile unsigned int *)0x7E215044)
#define AUX_MU_IIR_REG      (( volatile unsigned int *)0x7E215048)
#define AUX_MU_LCR_REG      (( volatile unsigned int *)0x7E21504C)
#define AUX_MU_MCR_REG      (( volatile unsigned int *)0x7E215050)
#define AUX_MU_LSR_REG      (( volatile unsigned int *)0x7E215054)
#define AUX_MU_MSR_REG      (( volatile unsigned int *)0x7E215058)
#define AUX_MU_SCRATCH      (( volatile unsigned int *)0x7E21505C)
#define AUX_MU_CNTL_REG     (( volatile unsigned int *)0x7E215060)
#define AUX_MU_STAT_REG     (( volatile unsigned int *)0x7E215064)
#define AUX_MU_BAUD_REG     (( volatile unsigned int *)0x7E215068)

#define AUX_SPI1_CNTL0_REG  (( volatile unsigned int *)0x7E215080)
#define AUX_SPI1_CNTL1_REG  (( volatile unsigned int *)0x7E215084)
#define AUX_SPI1_STAT_REG   (( volatile unsigned int *)0x7E215088)

#define AUX_SPI1_IO_REG     (( volatile unsigned int *)0x7E215090)
#define AUX_SPI1_PEEK_REG   (( volatile unsigned int *)0x7E215094)

#define AUX_SPI2_CNTL0_REG  (( volatile unsigned int *)0x7E2150C0)
#define AUX_SPI2_CNTL1_REG  (( volatile unsigned int *)0x7E2150C4)
#define AUX_SPI2_STAT_REG   (( volatile unsigned int *)0x7E2150C8)

#define AUX_SPI2_IO_REG     (( volatile unsigned int *)0x7E2150D0)
#define AUX_SPI2_PEEK_REG   (( volatile unsigned int *)0x7E2150D4)

#endif