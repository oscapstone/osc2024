#include "mini_uart.h"
#include "bootloader.h"

void cmd_help()
{
    uart_send_string("Usage:\r\n");
    uart_send_string("help\t: print this help menu\r\n");
    uart_send_string("hello\t: print Hello World!\r\n");
    uart_send_string("load\t: load kernel image\r\n");
}

void cmd_hello()
{
    uart_send_string("Hello World!\r\n");
}

void cmd_load()
{
    uart_send_string("Loading kernel image...\r\n");
    load_kernel_img();
}

void cmd_not_found()
{
    uart_send_string("shell: command not found\r\n");
}
