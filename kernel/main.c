#include "fdt.h"
#include "initramfs.h"
#include "uart.h"
#include "shell.h"

void main()
{
    uart_init();
    fdt_traverse(initramfs_callback);
    
    parse_initramfs();

    uart_puts("\nWelcome to kernel!\n");
    shell_start();
}