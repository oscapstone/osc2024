#include "io.h"
#include "uart.h"

char read_char()
{
    return uart_getc();
}

void print_char(char c)
{
    uart_send(c);
}

void print_string(char *s)
{
    uart_puts(s);
}

void print_h(int x) 
{
    uart_hex(x);
}