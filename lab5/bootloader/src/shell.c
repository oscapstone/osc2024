#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mailbox.h"
#include "../include/reboot.h"

#define BUFFER_SIZE 100

void shell()
{
    while (1) {
        char buffer[BUFFER_SIZE];
        uart_send_string("> ");
        read_command(buffer);
        parse_command(buffer);
    }
}

void read_command(char *buffer)
{
    int index = 0;
    while (1)
    {
        buffer[index] = uart_recv();
        uart_send(buffer[index]);
        if (buffer[index] == '\r')
        {
            uart_send('\n');
            break;
        }
        index++;
    }
    buffer[index] = '\0';
}

void parse_command(char *buffer)
{
    // First, we need to deal with '\n' at the end before '\0' in buffer
    
    // Then, use my_strcmp to find which command it can be used
    if (my_strcmp(buffer, "help") == 0) {
        help();
    } else if (my_strcmp(buffer, "hello") == 0) {
        hello();
    } else if (my_strcmp(buffer, "info") == 0) {
        get_board_revision();
        get_base_address();
    } else if (my_strcmp(buffer, "reboot") == 0) {
        uart_send_string("rebooting...\r\n");
        reset(1000);
    } else {
        uart_send_string("command ");
        uart_send_string(buffer);
        uart_send_string(" not found\r\n");
    }
}

void help()
{
    uart_send_string("help        print all available commands\r\n");
    uart_send_string("hello       print Hello World!\r\n");
    uart_send_string("info        print board revision and memory base address and size\r\n");
    uart_send_string("reboot      reboot the rpi3b+\r\n");
}

void hello()
{
    uart_send_string("Hello World!\r\n");
}

