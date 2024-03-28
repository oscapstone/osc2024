#include "initramfs.h"
#include "uart.h"
#include "shell.h"

void main()
{
    uart_init();

    parse_initramfs();

    shell_start();
}