#ifndef __MINI_UART_H__
#define __MINI_UART_H__

#include "base.h"
// #define MMU_BASE 0x3F000000 // corresponding to bus address 0x7E000000

#define AUX_ENABLES     (volatile unsigned int*)(MMU_BASE+0x00215004)
#define AUX_MU_IO_REG   (volatile unsigned int*)(MMU_BASE+0x00215040)
#define AUX_MU_IER_REG  (volatile unsigned int*)(MMU_BASE+0x00215044)
#define AUX_MU_IIR_REG  (volatile unsigned int*)(MMU_BASE+0x00215048)
#define AUX_MU_LCR_REG  (volatile unsigned int*)(MMU_BASE+0x0021504C)
#define AUX_MU_MCR_REG  (volatile unsigned int*)(MMU_BASE+0x00215050)
#define AUX_MU_LSR_REG  (volatile unsigned int*)(MMU_BASE+0x00215054)
#define AUX_MU_MSR_REG  (volatile unsigned int*)(MMU_BASE+0x00215058)
#define AUX_MU_SCRATCH  (volatile unsigned int*)(MMU_BASE+0x0021505C)
#define AUX_MU_CNTL_REG (volatile unsigned int*)(MMU_BASE+0x00215060)
#define AUX_MU_BAUD_REG (volatile unsigned int*)(MMU_BASE+0x00215068)

void uart_init();
char uart_recv();
void uart_send(unsigned int c);
void uart_sendc(const char c);
void uart_sends(const char* s);
void uart_sendh(unsigned int n);

#endif