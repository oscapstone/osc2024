#ifndef __AUX_H__
#define __AUX_H__

#include "mmio.h"

#define AUX_BASE        (MMIO_BASE + 0x215000)

/* Auxilary mini UART registers */
#define AUX_IRQ         ((volatile unsigned int*)(AUX_BASE + 0x00))
#define AUX_ENABLES     ((volatile unsigned int*)(AUX_BASE + 0x04))		// Auxiliary enables    		| 3
#define AUX_MU_IO       ((volatile unsigned int*)(AUX_BASE + 0x40))		// I/O Data             		| 8
#define AUX_MU_IER      ((volatile unsigned int*)(AUX_BASE + 0x44))		// Interrupt Enable Register    | 8
#define AUX_MU_IIR      ((volatile unsigned int*)(AUX_BASE + 0x48))		// Interrupt Identify Register 	| 8
#define AUX_MU_LCR      ((volatile unsigned int*)(AUX_BASE + 0x4C))		// Line Control Register    	| 8
#define AUX_MU_MCR      ((volatile unsigned int*)(AUX_BASE + 0x50))		// Modem Control Register  		| 8
#define AUX_MU_LSR      ((volatile unsigned int*)(AUX_BASE + 0x54))		// Line Status Register 		| 8
#define AUX_MU_MSR      ((volatile unsigned int*)(AUX_BASE + 0x58))		// Modem Status Register    	| 8
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(AUX_BASE + 0x5C))		// Scratch              		| 8
#define AUX_MU_CNTL     ((volatile unsigned int*)(AUX_BASE + 0x60))	 	// Extra Control        		| 8
#define AUX_MU_STAT     ((volatile unsigned int*)(AUX_BASE + 0x64))		// Extra Status         		| 32
#define AUX_MU_BAUD     ((volatile unsigned int*)(AUX_BASE + 0x68))		// Baudrate             		| 16

#define AUX_SPI0_CNTL0  ((volatile unsigned int*)(AUX_BASE + 0x80))
#define AUX_SPI0_CNTL1  ((volatile unsigned int*)(AUX_BASE + 0x84))
#define AUX_SPI0_STAT   ((volatile unsigned int*)(AUX_BASE + 0x88))
#define AUX_SPI0_IO     ((volatile unsigned int*)(AUX_BASE + 0x90))
#define AUX_SPI0_PEEK   ((volatile unsigned int*)(AUX_BASE + 0x94))
#define AUX_SPI1_CNTL0  ((volatile unsigned int*)(AUX_BASE + 0xC0))
#define AUX_SPI1_CNTL1  ((volatile unsigned int*)(AUX_BASE + 0xC4))
#define AUX_SPI1_STAT   ((volatile unsigned int*)(AUX_BASE + 0xC8))
#define AUX_SPI1_IO     ((volatile unsigned int*)(AUX_BASE + 0xD0))
#define AUX_SPI1_PEEK   ((volatile unsigned int*)(AUX_BASE + 0xD4))


#define aux_set_rx_interrupts() { *AUX_MU_IER |= 0x1; }
#define aux_clr_rx_interrupts() { *AUX_MU_IER &= ~(0x1); }
#define aux_set_tx_interrupts() { *AUX_MU_IER |= 0x2; }
#define aux_clr_tx_interrupts() { *AUX_MU_IER &= ~(0x2); }


#endif