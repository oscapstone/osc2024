#ifndef _DEF_UART
#define _DEF_UART

// Use UART physical address instead of bus address 
#define UART_PHYSICAL_BASE 0x3F000000
#define UART_OFFSET(offset) ((unsigned int *) (UART_PHYSICAL_BASE + offset))

// UART auxiliary registers (MMIO)
// 不需要 volatile (?)，因為這些數值肯定不會被更動，用 register 暫存的資料也可以
#define UART_AUXENB             UART_OFFSET(0x00215004)
#define UART_AUX_MU_IO_REG      UART_OFFSET(0x00215040)
#define UART_AUX_MU_IER_REG     UART_OFFSET(0x00215044)
#define UART_AUX_MU_IIR_REG     UART_OFFSET(0x00215048)
#define UART_AUX_MU_LCR_REG     UART_OFFSET(0x0021504C)
#define UART_AUX_MU_MCR_REG     UART_OFFSET(0x00215050)
#define UART_AUX_MU_LSR_REG     UART_OFFSET(0x00215054)
#define UART_AUX_MU_CNTL_REG    UART_OFFSET(0x00215060)
#define UART_AUX_MU_BAUD        UART_OFFSET(0x00215068)
#define UART_IRQs1_PENDING      UART_OFFSET(0x0000B204)
#define UART_IRQs1              UART_OFFSET(0x0000B210)

// TODO: what's the more appropriate name for these constant?
#define UART_GPFSEL0            UART_OFFSET(0x00200000)
#define UART_GPFSEL1            UART_OFFSET(0x00200004)
#define UART_GPPUD              UART_OFFSET(0x00200094)
#define UART_GPPUDCLK0          UART_OFFSET(0x00200098)
#define UART_GPPUDCLK1          UART_OFFSET(0x0020009C)

#define PM_WDOG_MAGIC           0x5A000000
#define PM_RSTC_FULLRST         0x00000020
#define PM_RSTC                 (unsigned int *) 0x3F10001C
#define PM_RSTS                 (unsigned int *) 0x3F100020
#define PM_WDOG                 (unsigned int *) 0x3F100024

void uart_init(void);   // initialization
void uart_init_buffer(void);    // initialize uart w/r buffer
void uart_send(unsigned int);   // send character over seiral line
void uart_puts(char*);   // write data
char uart_getc(void);   // read data
void uart_hex(unsigned int);
void uart_putints(int);
void uart_putuints(unsigned int);
void uart_putlong(long);
void uart_async_puts(char *);

void uart_tx_handler(void);

void uart_disable_rx_interrupt(void);
void uart_disable_tx_interrupt(void);

void uart_enable_rx_interrupt(void);
void uart_enable_tx_interrupt(void);

#include "ring_buffer.h"
#define RBUFFER_SIZE 100    // ring buffer size
static rbuffer_t tx_buf;    // Tx ring buffer
static rbuffer_t rx_buf;    // Rx ring buffer

#endif