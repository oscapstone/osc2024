#ifndef __UART_H__
#define __UART_H__

#include "mini_uart.h"

#define     uart_init       mini_uart_init
#define     uart_getc       mini_uart_getc
#define     uart_get        mini_uart_getb
#define     uart_put        mini_uart_putc
#define     uart_str        mini_uart_puts
#define     uart_line       mini_uart_putln
#define     uart_endl       mini_uart_endl

#define     uart_hex        mini_uart_hex
#define     uart_hexl       mini_uart_hexl

#define     uart_printf     mini_uart_printf

#endif