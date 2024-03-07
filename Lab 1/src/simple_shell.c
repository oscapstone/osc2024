#include <stddef.h>
#include "mini_uart.h"
#include "mailbox.h"
#include "power.h"
#include "string.h"


#define BUFFER_MAX_SIZE     256

char *read_command(char *buffer);
void parse_command(char *buffer);

void print_help();


void simple_shell() 
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
    mini_uart_puts("\r\n");
    mini_uart_puts("# ");
    do {
        r = mini_uart_getc();
        mini_uart_putc(r);
        buffer[index++] = r;
    } while (index < (BUFFER_MAX_SIZE - 1) && r != '\n');
    if (r == '\n') index--;
    buffer[index] = '\0';
    mini_uart_puts("\r\n");
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

    else if (str_eql(buffer, "mailbox")) {
        mailbox_get_board_revision();
        mini_uart_puts("\r\n");
        mailbox_get_arm_memory();
        mini_uart_puts("\r\n");
        mailbox_get_vc_info();
    }

    else if (str_eql(buffer, "reboot")) {
        mini_uart_putln("rebooting...");
        reset(1000);
    }

    else {
        mini_uart_puts("not a command: ");
        mini_uart_puts(buffer);
        mini_uart_puts("\r\n");
    }
}


void print_help()
{
    mini_uart_puts("help\t:");
    mini_uart_putln("print this help menu");
    mini_uart_puts("hello\t:");
    mini_uart_putln("print Hello World!");
    mini_uart_puts("mailbox\t:");
    mini_uart_putln("the mailbox hardware info");
    mini_uart_puts("reboot\t:");
    mini_uart_putln("reboot the device");
}

