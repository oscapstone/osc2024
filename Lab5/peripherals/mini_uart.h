#ifndef	_MINI_UART_H
#define	_MINI_UART_H

#include "base.h"

#define AUX_IRQ         ((volatile unsigned int*)(PBASE+0x00215000))
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



void uart_init (void);
char uart_recv (void);
void uart_send (char c);
void uart_send_string(char* str);
// For hexadecimal.
void uart_send_uint(unsigned long data);
// For decimal.
void uart_send_int(unsigned long data);
void enable_uart_interrupt(void);
void disable_uart_interrupt(void);
void handle_uart_interrupt(void* data);
int uart_recv_async(char* buf, int buf_size);
// void uart_send_async(const char* data, int len);
void uart_send_async(char c);

#endif  /*_MINI_UART_H */