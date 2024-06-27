#ifndef UART_H
#define UART_H

#include "../include/gpio.h"
#include "../include/shell.h"
#include "../include/command.h"
#include "../include/my_string.h"
#include "../include/my_stdlib.h"

#define MMIO_BASE       0x3F000000


/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))
#define AUX_IRQ         29

#define CORE0_INT_SRC       0x40000060
#define ENABLE_IRQS_1       0x3F00B210
#define IRQ_PENDING_1       0x3F00B204

#define TMP_BUFFER_LEN 1024

#define BUFFER_SIZE (16*16)
#define MAX_ARGV_LEN 32

// Buffers and indices
extern char read_buffer[BUFFER_SIZE];
extern char write_buffer[BUFFER_SIZE];
extern int read_index_cur;
extern int read_index_tail;
extern int write_index_cur;
extern int write_index_tail;


void uart_init(void);
void uart_send(unsigned int c);
char uart_getc(void);
void uart_puts(const char *s);
char *get_string(void);
void uart_hex(unsigned int d);

void initialize_async_buffers(void);
void uart_send_async(char c);
char uart_getc_async(void);
void uart_puts_async(const char *str);
void get_string_async(char *buffer_async);

void uart_int(int num);
void uart_hex_64(uint64_t d);


#endif
