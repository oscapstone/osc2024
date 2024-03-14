#ifndef	_P_MINI_UART_H
#define	_P_MINI_UART_H

#include "base.h"

#define AUX_ENABLES     ((volatile unsigned int*)(PBASE+0x00215004))
#define AUX_MU_IO_REG   ((volatile unsigned int*)(PBASE+0x00215040))  // Used to write data to and read data from the uart FIFOs.
#define AUX_MU_IER_REG  ((volatile unsigned int*)(PBASE+0x00215044))  // Used to enable interrupts.
#define AUX_MU_IIR_REG  ((volatile unsigned int*)(PBASE+0x00215048))  // Shows the interrupt status.
#define AUX_MU_LCR_REG  ((volatile unsigned int*)(PBASE+0x0021504C))  // Controls the line data format and gives access to the baudrate register.
#define AUX_MU_MCR_REG  ((volatile unsigned int*)(PBASE+0x00215050))  // Controls the "modem" signals.
#define AUX_MU_LSR_REG  ((volatile unsigned int*)(PBASE+0x00215054))  // Shows data status.
#define AUX_MU_MSR_REG  ((volatile unsigned int*)(PBASE+0x00215058))  // Shows modem status.
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(PBASE+0x0021505C))  // Single byte storage.
#define AUX_MU_CNTL_REG ((volatile unsigned int*)(PBASE+0x00215060))  // Provides access to some extra useful and nice features not found on a normal 16550 uart.
#define AUX_MU_STAT_REG ((volatile unsigned int*)(PBASE+0x00215064))  // Provides a lot of useful information about the internal status of the mini uart not found on
                                                                      // a normal 16550 uart.
#define AUX_MU_BAUD_REG ((volatile unsigned int*)(PBASE+0x00215068))  // Allows direct access to the 16-bit wide baudrate counter.

#endif  /*_MINI_UART_H */