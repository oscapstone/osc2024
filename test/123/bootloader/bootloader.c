
#include "header/bootloader.h"
#include "header/uart.h"
#include "header/math.h"

void load_img(){
    char size_char = '0';
    int size = 0, exponent = 0;

    while (1)
    {
        size_char = uart_get_char();

        // stop sending
        if (size_char == '\n')
            break;
        // throw away junk bits in the buffer
        if (size_char < 48 || size_char > 57)
            continue;

        size += (size_char - '0') * pow(10, exponent);
        exponent++;
    }

    uart_send_string("size-check correct\r\n");

    char *kernel = (char *) 0x80000;

    for (int i = 0; i < size; i++)
        *kernel++ = uart_get_char();

    uart_send_string("kernel-loaded\r\n");
    
    asm volatile(
        "mov x0, x10\n"
        // "mov x1, x11\n"
        // "mov x2, x12\n"
        // "mov x3, x13\n"
        );

    asm volatile(
       "mov x30, 0x80000;"
       "ret;"
    );
}
