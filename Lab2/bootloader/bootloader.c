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
        //not 0 ~ 9
        if (size_char < 48 || size_char > 57)
            continue;
        //string to int
        size += (size_char - '0') * pow(10, exponent);
        exponent++;
    }

    uart_send_string("size-check correct\r\n");

    char *kernel = (char *) 0x80000;

    //send kernel8.img
    for (int i = 0; i < size; i++)
        *kernel++ = uart_get_char();

    uart_send_string("kernel-loaded\r\n");
    
    //get dbt address back
    asm volatile(
        "mov x0, x10\n"
        // "mov x1, x11\n"
        // "mov x2, x12\n"
        // "mov x3, x13\n"
        );

    asm volatile(
        //https://github.com/bztsrc/raspi3-tutorial/blob/master/14_raspbootin64/main.c
        // we must force an absolute address to branch to
        //x30:Link Register https://developer.arm.com/documentation/dui0801/l/Overview-of-AArch64-state/Link-registers?lang=en
       "mov x30, 0x80000;"
       "ret;"
    );
}
