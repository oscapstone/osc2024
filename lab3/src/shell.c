#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mailbox.h"
#include "../include/reboot.h"
#include "../include/cpio.h"
#include "../include/mem_utils.h"

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
    } else if (my_strcmp(buffer, "ls") == 0) {
        cpio_ls();
    } else if (my_strcmp(buffer, "cat") == 0) {
        cpio_cat();
    } else if (my_strcmp(buffer, "load") == 0) {
        cpio_load_program();
    } else if (my_strcmp(buffer, "malloc") == 0) {
        /* test malloc */
        char *tmp = malloc(4);
        if (!tmp) {
        	uart_send_string("HEAP overflow!!!\r\n");
        	return;
        }
        tmp[0] = '1';
        tmp[1] = '2';
        tmp[2] = '3';
        tmp[3] = '\0';
        uart_send_string(tmp);
        uart_send_string("\r\n");
        
        /* test strlen */
        uart_send_string("The length of tmp is: ");
        uart_hex(my_strlen(tmp));
        uart_send_string("\r\n");
    } 
    else {
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
    uart_send_string("ls          show all files in rootfs\r\n");
    uart_send_string("cat         print out the content of specific file\r\n");
    uart_send_string("load        load user program and execute\r\n");
    uart_send_string("malloc      try to print the content of malloc\r\n");
}

void hello()
{
    uart_send_string("Hello World!\r\n");
}

