
#include "io/uart.h"

void main() {

    // initial the UART
    uart_init();

    unsigned int size = 0;
    unsigned char *sizeBuffer = (unsigned char*) &size;

    // read size from uart
    for(int i = 0; i < 4; i++)
        sizeBuffer[i] = uart_get_char();

    unsigned int r = 500; while (r--) { asm volatile("nop"); }

    for (int i = 0; i < 4; i++)
        uart_send_char(sizeBuffer[i]);

    // read kernel to memory
    char *kernelPtr = (char*) 0x80000;
    while (size--) {
        *kernelPtr++ = uart_get_char();
    }

    uart_send_string(13, "kernel loaded");

    // make sure it send the result after jumping to kernel
    // because it is so fast that kernel will reinit uart again
    r = 1000; while (r--) { asm volatile("nop"); }

    // jump to kernel
    asm volatile(
        "mov    x1, 0x80000;"
        "br     x1"
    );
}


