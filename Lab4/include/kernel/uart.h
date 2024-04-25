#ifndef UART_H
#define UART_H

#include "kernel/gpio.h"
#include "kernel/INT.h"

#define MAX_BUF_LEN 512
#define MAX_ARGV_LEN 32

// check p.8
// Auxiliary Interrupt status
#define AUX_IRQ             ((volatile unsigned int*)(MMIO_BASE + 0x00215000))
// Auxiliary enables
#define AUX_ENABLE          ((volatile unsigned int*)(MMIO_BASE + 0x00215004))
// Mini Uart I/O Data
#define AUX_MU_IO_REG       ((volatile unsigned int*)(MMIO_BASE + 0x00215040))
// Mini Uart Interrupt Enable
#define AUX_MU_IER_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215044))
// Mini Uart Interrupt Identity
#define AUX_MU_IIR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215048))
// Mini Uart Line Control
#define AUX_MU_LCR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x0021504C))
// Mini Uart Modem Control
#define AUX_MU_MCR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215050))
// Mini Uart Line Status
#define AUX_MU_LSR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215054))
// Mini Uart Modem Status 
#define AUX_MU_MSR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215058))
// Mini Uart Scratch
#define AUX_MU_SCRATCH      ((volatile unsigned int*)(MMIO_BASE + 0x0021505C))
// Mini Uart Scratch
#define AUX_MU_CNTL_REG     ((volatile unsigned int*)(MMIO_BASE + 0x00215060))
// Mini Uart Extra Status
#define AUX_MU_STAT_REG     ((volatile unsigned int*)(MMIO_BASE + 0x00215064))
// Mini Uart Baudrate
#define AUX_MU_BAUD_REG     ((volatile unsigned int*)(MMIO_BASE + 0x00215068))

extern char read_buffer[MAX_BUF_LEN];
extern char write_buffer[MAX_BUF_LEN];
extern int read_index_cur;
extern int read_index_tail;
extern int write_index_cur;
extern int write_index_tail;

void uart_init (void);
void uart_putc(unsigned char c);
unsigned char uart_getc();
unsigned int uart_puts(const char* str);
// output fixed length string, assuming length provided is correct
void uart_puts_fixed(const char *str, int len);
// binary to hex, only for mailbox
void uart_b2x(unsigned int b);
// for 64 bits universal version
void uart_b2x_64(unsigned long long b);
void uart_itoa(int num);
// this is for returning a printable string obtained from user, usually for file name
int uart_gets(char *buf, char **argv);
int uart_get_fn(char *buf);

int uart_irq_gets(char *buf);

void uart_irq_on();
void uart_irq_off();
// Get all string from the buffer, return value is string length
int uart_irq_getc();
void uart_irq_putc(unsigned char c);
void uart_irq_puts(const char *str);

#endif