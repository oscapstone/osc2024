
#include "bootloader.h"
#include "mini_uart.h"
#include "math.h"

void load_img(){
    char size_char = '0';
    int size = 0, exponent = 0;

    while (1)
    {
        size_char = uart_recv(); //send_img.py inform the size of image file

        // stop sending
        if (size_char == '\n')
            break;
        // throw away junk bits in the buffer
        if (size_char < 48 || size_char > 57)
            continue;

        size += (size_char - '0') * pow(10, exponent);
        exponent++;
    }

    uart_send_string("successfully get the size of image file\r\n");

    char *kernel = (char *) 0x80000;

    for (int i = 0; i < size; i++)
        *kernel++ = uart_recv();

    uart_send_string("kernel loaded\r\n");
    
    asm volatile(
        "mov x0, x10\n" //move data from x10 to x0
        "mov x1, x11\n"
        "mov x2, x12\n"
        "mov x3, x13\n"
        );

    asm volatile(
       "mov x30, 0x80000;"
       "ret;"
    );
}
