#ifndef	_P_MINI_UART_H
#define	_P_MINI_UART_H

#include "peripherals/base.h"

#define AUX_ENABLES     ((volatile unsigned int *)(PBASE+0x00215004))
#define AUX_MU_IO_REG   ((volatile unsigned int *)(PBASE+0x00215040))
#define AUX_MU_IER_REG  ((volatile unsigned int *)(PBASE+0x00215044))
#define AUX_MU_IIR_REG  ((volatile unsigned int *)(PBASE+0x00215048))
#define AUX_MU_LCR_REG  ((volatile unsigned int *)(PBASE+0x0021504C))
#define AUX_MU_MCR_REG  ((volatile unsigned int *)(PBASE+0x00215050))
#define AUX_MU_LSR_REG  ((volatile unsigned int *)(PBASE+0x00215054))
#define AUX_MU_MSR_REG  ((volatile unsigned int *)(PBASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int *)(PBASE+0x0021505C))
#define AUX_MU_CNTL_REG ((volatile unsigned int *)(PBASE+0x00215060))
#define AUX_MU_STAT_REG ((volatile unsigned int *)(PBASE+0x00215064))
#define AUX_MU_BAUD_REG ((volatile unsigned int *)(PBASE+0x00215068))

/* for async I/O */
#define ARM_IRQ_REG_BASE ((volatile unsigned int*)(PBASE + 0x0000b000))
#define IRQ_PENDING_1 	 ((volatile unsigned int*)(PBASE + 0x0000b204))
#define ENB_IRQS1 		 ((volatile unsigned int*)(PBASE + 0x0000b210))
#define DISABLE_IRQS1 	 ((volatile unsigned int*)(PBASE + 0x0000b21c))

#define set_rx_interrupts() { *AUX_MU_IER_REG |= 0x1;}
#define clr_rx_interrupts() { *AUX_MU_IER_REG &= ~(0x1);}
#define set_tx_interrupts() { *AUX_MU_IER_REG |= 0x2;}
#define clr_tx_interrupts() { *AUX_MU_IER_REG &= ~(0x2);}

#define enable_aux_interrupts() {*ENB_IRQS1 = (1 << 29);}
#define disable_aux_interrupts() {*DISABLE_IRQS1 = (1 << 29);}

#endif  /*_P_MINI_UART_H */