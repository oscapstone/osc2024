#include "shell.h"
#include "uart.h"

extern char __start;
extern char __end;

int relocated = 1;

void relocate(char *arg)
{
    unsigned long bootloader_size = (&__end - &__start);
    char *src = (char *)&__start;
    char *dst = (char *)0x60000;

    unsigned long bootloader_ptr = 0;
    while (bootloader_size--) {
        dst[bootloader_ptr] = src[bootloader_ptr];
        bootloader_ptr++;
    }

    void (*relocated_main)(char *) = (void (*)(char *))dst;
    relocated_main(arg);
}

void main(char *arg)
{
    uart_init();

    if (relocated) {
        relocated = 0;
        relocate(arg);
    }
    uart_puts("\x1b[2J\x1b[H");
    uart_puts("\nBootloader Start.\n");

    bootloader_shell();
}