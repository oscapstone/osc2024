#pragma once

#include <stdint.h>

#include "hardware.h"
#include "vfs.h"

#define UART_BUF_SIZE 1024

// Special characters
#define BACKSPACE '\b'
#define DELETE 127
#define ESC 27
#define NEWLINE '\n'
#define TAB '\t'
#define CARRIAGE_RETURN '\r'

// uart_log types
#define INFO 0
#define WARN 1
#define TEST 2
#define BUDD 10
#define CACH 11
#define VFS 12
#define MMAP 13
#define SCALL 253
#define DEBUG 254
#define ERR 255

void init_uart();
char uart_getc();
void uart_putc(char c);
void uart_puts(const char *s);
void uart_clear();
void uart_hex(unsigned long h);
void uart_addr_range(uintptr_t start, uintptr_t end);
void uart_simple_hex(unsigned int h);
void uart_dec(unsigned int h);
void uart_log(int type, const char *msg);

void enable_uart_tx_interrupt();
void disable_uart_tx_interrupt();
void enable_uart_rx_interrupt();
void disable_uart_rx_interrupt();

void uart_tx_irq_handler();
void uart_rx_irq_handler();

void uart_async_read(char *buf, int len);
void uart_async_write(const char *s);

file_operations *init_dev_uart();
int dev_uart_write(file *file, const void *buf, size_t len);
int dev_uart_read(file *file, void *buf, size_t len);
int dev_uart_open(vnode *file_node, file *target);
int dev_uart_close(file *file);