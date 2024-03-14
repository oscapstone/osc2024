#ifndef _P_MINI_UART_H
#define _P_MINI_UART_H

#include "base.h"

// Define Mini UART register addresses based on the peripheral base address (PBASE)
#define AUX_ENABLES     (PBASE + 0x00215004) // Auxiliary Enables: Used to enable/disable the Mini UART and SPI1/SPI2
#define AUX_MU_IO_REG   (PBASE + 0x00215040) // Mini UART I/O Data: Used for reading/writing data to/from the Mini UART
#define AUX_MU_IER_REG  (PBASE + 0x00215044) // Mini UART Interrupt Enable: Used to enable/disable interrupts
#define AUX_MU_IIR_REG  (PBASE + 0x00215048) // Mini UART Interrupt Identify: Used to identify and clear interrupts
#define AUX_MU_LCR_REG  (PBASE + 0x0021504C) // Mini UART Line Control: Used to set data format (e.g., number of data bits, parity)
#define AUX_MU_MCR_REG  (PBASE + 0x00215050) // Mini UART Modem Control: Used to control modem signals (not applicable for the Mini UART)
#define AUX_MU_LSR_REG  (PBASE + 0x00215054) // Mini UART Line Status: Used to check the status of data transmission and reception
#define AUX_MU_MSR_REG  (PBASE + 0x00215058) // Mini UART Modem Status: Used to check the status of modem signals (not applicable for the Mini UART)
#define AUX_MU_SCRATCH  (PBASE + 0x0021505C) // Mini UART Scratch: A general-purpose scratch register
#define AUX_MU_CNTL_REG (PBASE + 0x00215060) // Mini UART Extra Control: Used to control additional features like flow control
#define AUX_MU_STAT_REG (PBASE + 0x00215064) // Mini UART Extra Status: Used to check additional status information
#define AUX_MU_BAUD_REG (PBASE + 0x00215068) // Mini UART Baud Rate: Used to set the baud rate for data transmission

#endif /*_P_MINI_UART_H */