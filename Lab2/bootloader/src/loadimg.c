#include "uart0.h"

void loadimg() {
    long long address = 0x80000;

    if (address == -1) {
        return;
    }

    uart_printf("Send image via UART now!\n");

    // big endian, load size
    int img_size = 0, i;
    for (i = 0; i < 4; i++) {
        img_size <<= 8;
        img_size |= (int)uart_recv_raw();
    }
    // big endian, load checksum
    int img_checksum = 0;
    for (i = 0; i < 4; i++) {
        img_checksum <<= 8;
        img_checksum |= (int)uart_recv_raw();
    }
    char *kernel = (char *)address;
    for (i = 0; i < img_size; i++) {
        char b = uart_recv_raw();
        *(kernel + i) = b;
        img_checksum -= (int)b;
    }

    if (img_checksum != 0) {
        uart_printf("Failed!\n");
    }
    else {
        uart_printf("Success!\n");
        void (*start_os)(void) = (void *)kernel;
        // __asm__("mov x0, x25");
        start_os();
    }
}