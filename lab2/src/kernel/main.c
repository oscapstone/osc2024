#include "dtb.h"
#include "malloc.h"
#include "shell.h"
#include "uart.h"

void main(char *arg)
{
    uart_init();

    fdt_traverse(initramfs_callback, arg);

    uart_puts("\x1b[2J\x1b[H");
    uart_puts("Hello, kernel World!\n");
    uart_puts("DTB base: ");
    uart_hex((unsigned long)arg);

    char *string = simple_malloc(8);

    shell();
}