#ifndef _AUX_H
#define _AUX_H

#include "peripherals/mmio.h"
#include "types.h"

#define AUX_BASE (MMIO_BASE + 0x215000)

#define AUX_IRQ \
  ((volatile uint32_t *)(AUX_BASE + 0x00))  // Auxiliary interrupt register
#define AUX_ENABLES \
  ((volatile uint32_t *)(AUX_BASE + 0x04))  // Auxiliary enables register
#define AUX_MU_IO \
  ((volatile uint32_t *)(AUX_BASE + 0x40))  // Mini UART I/O data register
#define AUX_MU_IER                  \
  ((volatile uint32_t *)(AUX_BASE + \
                         0x44))  // Mini UART interrupt enable register
#define AUX_MU_IIR                  \
  ((volatile uint32_t *)(AUX_BASE + \
                         0x48))  // Mini UART interrupt identify register
#define AUX_MU_LCR \
  ((volatile uint32_t *)(AUX_BASE + 0x4C))  // Mini UART line control register
#define AUX_MU_MCR \
  ((volatile uint32_t *)(AUX_BASE + 0x50))  // Mini UART modem control register
#define AUX_MU_LSR \
  ((volatile uint32_t *)(AUX_BASE + 0x54))  // Mini UART line status register
#define AUX_MU_MSR \
  ((volatile uint32_t *)(AUX_BASE + 0x58))  // Mini UART modem status register
#define AUX_MU_SCRATCH \
  ((volatile uint32_t *)(AUX_BASE + 0x5C))  // Mini UART scratch register
#define AUX_MU_CNTL \
  ((volatile uint32_t *)(AUX_BASE + 0x60))  // Mini UART extra control register
#define AUX_MU_STAT \
  ((volatile uint32_t *)(AUX_BASE + 0x64))  // Mini UART extra status register
#define AUX_MU_BAUD \
  ((volatile uint32_t *)(AUX_BASE + 0x68))  // Mini UART baudrate register
#define AUX_SPI0_CNTL0 \
  ((volatile uint32_t *)(AUX_BASE + 0x80))  // SPI0 control register 0
#define AUX_SPI0_CNTL1 \
  ((volatile uint32_t *)(AUX_BASE + 0x84))  // SPI0 control register 1
#define AUX_SPI0_STAT \
  ((volatile uint32_t *)(AUX_BASE + 0x88))  // SPI0 status register.
#define AUX_SPI0_IO \
  ((volatile uint32_t *)(AUX_BASE + 0x90))  // SPI0 data register.
#define AUX_SPI0_PEEK \
  ((volatile uint32_t *)(AUX_BASE + 0x94))  // SPI0 data peek register
#define AUX_SPI1_CNTL0 \
  ((volatile uint32_t *)(AUX_BASE + 0xC0))  // SPI1 control register 0
#define AUX_SPI1_CNTL1 \
  ((volatile uint32_t *)(AUX_BASE + 0xC4))  // SPI1 control register 1
#define AUX_SPI1_STAT \
  ((volatile uint32_t *)(AUX_BASE + 0xC8))  // SPI1 status register
#define AUX_SPI1_IO \
  ((volatile uint32_t *)(AUX_BASE + 0xD0))  // SPI1 data register
#define AUX_SPI1_PEEK \
  ((volatile uint32_t *)(AUX_BASE + 0xD4))  // SPI1 data peek register

#endif