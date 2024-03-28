#include "header/shell.h"
#include "../header/utils.h"
#include "../header/mini_uart.h"
#include "../header/mailbox.h"
#include "../header/reboot.h"
#include <stddef.h>

#define BUFFER_MAX_SIZE 256
#define COMMNAD_LENGTH_MAX 20

void help()
{
    uart_send_string("help    : print this help menu\r\n");
    uart_send_string("hello   : print Hello World!\r\n");
    uart_send_string("reboot  : reboot the device\r\n");
    uart_send_string("info    : the mailbox hardware info\r\n");
}

void hello()
{
    uart_send_string("Hello World!\r\n");
}

void reboot()
{
    uart_send_string("rebooting...\r\n");
    reset(1000);
}

void info()
{
    get_board_revision();
    get_arm_memory();
}

void read_command(char *buffer)
{
    size_t index = 0;
    
    while (1)
    {
        buffer[index] = uart_recv();
        uart_send(buffer[index]);
        if (buffer[index] == '\n')
            break;

        index++;
    }
    
    buffer[index + 1] = '\0';
}

void parse_command(char *buffer)
{
    utils_newline2end(buffer);
    uart_send('\r');

    if (buffer[0] == '\0')
        return;
    else if (utils_str_compare(buffer, "help") == 0)
        help();
    else if (utils_str_compare(buffer, "hello") == 0)
        hello();
    else if (utils_str_compare(buffer, "reboot") == 0)
        reboot();
    else if (utils_str_compare(buffer, "info") == 0)
        info();
    else
        uart_send_string("commnad not found\r\n");
}

void shell()
{
    while (1)
    {
        char buffer[BUFFER_MAX_SIZE];
        uart_send_string("# ");
        read_command(buffer);
        parse_command(buffer);
    }
}
