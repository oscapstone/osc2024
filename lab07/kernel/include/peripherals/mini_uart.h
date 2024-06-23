#ifndef __PERIPHERALS_MINI_UART_H__
#define __PERIPHERALS_MINI_UART_H__

#include "peripherals/base.h"

#define AUX_ENABLES     ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO_REG   ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER_REG  ((volatile unsigned int*)(MMIO_BASE+0x00215044))    // Interrupt Enable Register (BCM2835 page. 12)
#define AUX_MU_IIR_REG  ((volatile unsigned int*)(MMIO_BASE+0x00215048))    // Shows the interrupt status. (BCM2835 page. 13)
#define AUX_MU_LCR_REG  ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR_REG  ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR_REG  ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR_REG  ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL_REG ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT_REG ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD_REG ((volatile unsigned int*)(MMIO_BASE+0x00215068))


#define ENABLE_RCV_IRQ   (1 << 0)
#define ENABLE_TRANS_IRQ (1 << 1)

#define AUX_MU_IIR_REG_READ     (1 << 2)
#define AUX_MU_IIR_REG_WRITE    (1 << 1)

#endif