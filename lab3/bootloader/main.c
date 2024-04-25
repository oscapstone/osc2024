#include <stdlib.h>

#include "bsp/uart.h"

int main() {
    uart_init();
    uart_puts("\nMini UART Bootloader! Please send the kernel image...\n");

    

    // Get kernel image size
    char buf[10] = {0};
    while(1) {
        buf[0] = uart_recv();
        if(buf[0]> '9' || buf[0] < '0') continue;
        break;
    }
    for (int i = 1; i < 16; i++) {
        buf[i] = uart_recv();
        if (buf[i] == '\n') {
            buf[i] = '\0';
            break;
        }
    }

    uart_puts("Kernel size: ");
    uart_puts(buf);
    uart_puts(" bytes\n");

    uart_puts("Loading the kernel image...\n");
    unsigned int size = atoi(buf);
    uart_puts("Kernel image size: ");
    uart_hex(size);
    char *kernel = (char *)0x80000;
    uart_puts("\nBefore from: ");
    uart_hex(*kernel);
    while (size--) {
        *kernel++ = uart_recv();
    }
    uart_puts("\nAfter to: ");
    uart_hex(*kernel);
    uart_puts("\n");

    asm volatile(
        ""
        "mov x0, x10;"              // move back dtb_base to x0
        "mov x30, 0x80000; ret;");  // Jump to the new kernel

    return 0;
}
