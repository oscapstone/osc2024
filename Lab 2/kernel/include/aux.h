#ifndef __AUX_H__
#define __AUX_H__

#include "mmio.h"

#define AUX_BASE        (MMIO_BASE + 0x215000)

/* Auxilary mini UART registers */
#define AUX_IRQ         ((volatile unsigned int*)(AUX_BASE + 0x00000000))
#define AUX_ENABLES     ((volatile unsigned int*)(AUX_BASE + 0x00000004))		// Auxiliary enables    		| 3
#define AUX_MU_IO       ((volatile unsigned int*)(AUX_BASE + 0x00000040))		// I/O Data             		| 8
#define AUX_MU_IER      ((volatile unsigned int*)(AUX_BASE + 0x00000044))		// Interrupt Enable Register    | 8
#define AUX_MU_IIR      ((volatile unsigned int*)(AUX_BASE + 0x00000048))		// Interrupt Identify Register 	| 8
#define AUX_MU_LCR      ((volatile unsigned int*)(AUX_BASE + 0x0000004C))		// Line Control Register    	| 8
#define AUX_MU_MCR      ((volatile unsigned int*)(AUX_BASE + 0x00000050))		// Modem Control Register  		| 8
#define AUX_MU_LSR      ((volatile unsigned int*)(AUX_BASE + 0x00000054))		// Line Status Register 		| 8
#define AUX_MU_MSR      ((volatile unsigned int*)(AUX_BASE + 0x00000058))		// Modem Status Register    	| 8
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(AUX_BASE + 0x0000005C))		// Scratch              		| 8
#define AUX_MU_CNTL     ((volatile unsigned int*)(AUX_BASE + 0x00000060))	 	// Extra Control        		| 8
#define AUX_MU_STAT     ((volatile unsigned int*)(AUX_BASE + 0x00000064))		// Extra Status         		| 32
#define AUX_MU_BAUD     ((volatile unsigned int*)(AUX_BASE + 0x00000068))		// Baudrate             		| 16

#define AUX_SPI0_CNTL0  ((volatile unsigned int*)(AUX_BASE + 0x00000080))
#define AUX_SPI0_CNTL1  ((volatile unsigned int*)(AUX_BASE + 0x00000084))
#define AUX_SPI0_STAT   ((volatile unsigned int*)(AUX_BASE + 0x00000088))
#define AUX_SPI0_IO     ((volatile unsigned int*)(AUX_BASE + 0x00000090))
#define AUX_SPI0_PEEK   ((volatile unsigned int*)(AUX_BASE + 0x00000094))
#define AUX_SPI1_CNTL0  ((volatile unsigned int*)(AUX_BASE + 0x000000C0))
#define AUX_SPI1_CNTL1  ((volatile unsigned int*)(AUX_BASE + 0x000000C4))
#define AUX_SPI1_STAT   ((volatile unsigned int*)(AUX_BASE + 0x000000C8))
#define AUX_SPI1_IO     ((volatile unsigned int*)(AUX_BASE + 0x000000D0))
#define AUX_SPI1_PEEK   ((volatile unsigned int*)(AUX_BASE + 0x000000D4))

#endif