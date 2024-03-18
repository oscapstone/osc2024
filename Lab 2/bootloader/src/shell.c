#include <stddef.h>
#include "mini_uart.h"
#include "string.h"


#define BUFFER_MAX_SIZE     64

char *read_command(char *buffer);
void parse_command(char *buffer);

void print_help();


void shell() 
{
    while (1)
    {
        char buffer[BUFFER_MAX_SIZE];
        read_command(buffer);
        parse_command(buffer);
    }
}


char *read_command(char *buffer)
{
    size_t index = 0;
    char r = 0;
    mini_uart_putln("");
    mini_uart_puts("$ ");
    do {
        r = mini_uart_getc();
        mini_uart_putc(r);
        buffer[index++] = r;
    } while (index < (BUFFER_MAX_SIZE - 1) && r != '\n');
    if (r == '\n') index--;
    buffer[index] = '\0';
    mini_uart_putln("");
    return buffer;
}


void parse_command(char *buffer)
{
    if (str_eql(buffer, "help")) {
        print_help();
    }

    else if (str_eql(buffer, "hello")) {
        mini_uart_putln("Hello world!");
    }

    else {
        mini_uart_puts("not a command: ");
        mini_uart_putln(buffer);
    }
}


void print_help()
{
    mini_uart_puts("help\t:");
    mini_uart_putln("print this help menu");
    mini_uart_puts("hello\t:");
    mini_uart_putln("print Hello World!");
}

