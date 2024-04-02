#include "initramfs.h"
#include "uart.h"
#include "shell.h"

void main()
{
    uart_init();

    parse_initramfs();

    uart_puts("\nWelcome to kernel!\n");
    shell_start();
}