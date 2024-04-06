#include "fdt.h"
#include "initramfs.h"
#include "uart.h"
#include "shell.h"

void main()
{
    uart_init();

    fdt_traverse(initramfs_callback);

    uart_puts("\nWelcome to kernel!\n");
    shell_start();
}