#include "io.h"
#include "bsp/uart.h"

char read_c() { return uart_getc(); }

void print_char(char c) { return uart_send(c); }
void print_string(char *s) { return uart_puts(s); }

void print_h(int x) { uart_hex(x); }
