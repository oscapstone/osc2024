#include "load.h"
#include "uart.h"

void load()
{
    char *kernel = (char *)0x80000, c, sz[64] = {};
    int idx = 0;
    while (1) {
        c = uart_getc();
        if (c == '\n') {
            sz[idx] = '\0';
            break;
        }
        sz[idx++] = c;
    }

    // sz to int
    int size = 0, kn_ptr = 0;
    for (int i = 0; sz[i] != '\0'; i++) {
        size = size * 10 + (int)(sz[i] - '0');
    }
    uart_puts(sz);

    while (size--)
        kernel[kn_ptr++] = uart_getc();

    uart_puts("\nKernel loaded...\n");
    int r = 1000;
    while (r--) {
        asm volatile("nop");
    }

    void (*kernel_main)(char *) = (void *)kernel;
    kernel_main("hello from bootloader\n");
}