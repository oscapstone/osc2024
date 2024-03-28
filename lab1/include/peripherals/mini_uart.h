#ifndef	_P_MINI_UART_H
#define	_P_MINI_UART_H

#include "peripherals/base.h"

#define AUX_ENABLES     (PBASE+0x00215004)
// [0] Mini UART enable, if set, enable mini UART

#define AUX_MU_IO_REG   (PBASE+0x00215040)
// read/write data from the uart fifos
// [7:0] I/O data  

#define AUX_MU_IER_REG  (PBASE+0x00215044)
// enable interrupts
// [7:0] MS 9 bits

#define AUX_MU_IIR_REG  (PBASE+0x00215048)
// show interrupt status

#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
// controls the line data format and gives access to baudrate reg

#define AUX_MU_MCR_REG  (PBASE+0x00215050)
// controls the 'modem' signal

#define AUX_MU_LSR_REG  (PBASE+0x00215054)
// show the data status

#define AUX_MU_MSR_REG  (PBASE+0x00215058)
// show the 'modem' status

#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
// single byte storage

#define AUX_MU_CNTL_REG (PBASE+0x00215060)
// provides access to exctra features

#define AUX_MU_STAT_REG (PBASE+0x00215064)
// provide useful info about internal status of the mini uart

#define AUX_MU_BAUD_REG (PBASE+0x00215068)
// allows direct access to the 16-bit wide baudrate counter

#endif  /*_P_MINI_UART_H */