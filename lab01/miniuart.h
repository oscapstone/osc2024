// miniuart.h
#ifndef MINIUART_H
#define MINIUART_H

#define UART_BASE 0x3F000000  

// Define volatile pointers to Mini UART registers
//BCM2837 ARM Peripherals Page 8
#define AUX_ENABLE      ((volatile unsigned int*)(UART_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(UART_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(UART_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(UART_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(UART_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(UART_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(UART_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(UART_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(UART_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(UART_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(UART_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(UART_BASE+0x00215068))

void miniUARTInit();
char miniUARTRead();
void miniUARTWrite(char c);
void miniUARTWriteString(const char* s);
void miniUARTWriteHex(unsigned int d);

#endif // MINIUART_H
