#ifndef	_P_MINI_UART_H
#define	_P_MINI_UART_H

#include "peripherals/base.h"

#define AUX_ENABLES     (PBASE+0x00215004)
#define AUX_MU_IO_REG   (PBASE+0x00215040)
#define AUX_MU_IER_REG  (PBASE+0x00215044)
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
#define AUX_MU_MCR_REG  (PBASE+0x00215050)
#define AUX_MU_LSR_REG  (PBASE+0x00215054)
#define AUX_MU_MSR_REG  (PBASE+0x00215058)
#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
#define AUX_MU_CNTL_REG (PBASE+0x00215060)
#define AUX_MU_STAT_REG (PBASE+0x00215064)
#define AUX_MU_BAUD_REG (PBASE+0x00215068)
#define MAX_BUF_LEN 512

void uart_irq_puts(const char *str);
int uart_irq_gets(char *buf);
void uart_irq_on();
void uart_irq_off();

extern char read_buffer[MAX_BUF_LEN];
extern char write_buffer[MAX_BUF_LEN];
extern int read_index_cur;
extern int read_index_tail;
extern int write_index_cur;
extern int write_index_tail;

#endif  /*_P_MINI_UART_H */
