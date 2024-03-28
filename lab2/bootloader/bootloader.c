#include "header/bootloader.h"
#include "header/uart.h"

void load_img() {
    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    for(int i = 0; i < 4; i++) {
        size_buffer[i] = uart_get_char();
    }
    uart_send_string("\nsize-check correct");

    char *kernel = (char *) 0x80000;
    while(size--) {
        // *kernel = uart_get_char();
        *kernel = uart_get_char_for_load_img();
        kernel++;
    }
    asm volatile(
        // "mov x0, x12;"
        "mov x30, 0x80000;"
        "ret;"
    );
}