#include "bootloader.h"
#include "uart.h"
#include <stddef.h>
extern char *_dtb_ptr;
#define BUFFER_MAX_SIZE 256u

unsigned int str2uint_dec(const char *str)
{
    unsigned int value = 0u;
    while(str[0] != '.'){
        str++;
    }
    str++;

    while (*str)
    {
        value = value * 10u + (*str - '0');
        ++str;
    }
    return value;
}

void load_img(){
    unsigned char* kernel_addr = (unsigned char *)0x80000;

    uart_display_string("Start bootloading\n");

    char size_buffer[BUFFER_MAX_SIZE];
    size_t index = 0;

    while (1)
    {
        size_buffer[index] = uart_get_char();
        uart_send_char(size_buffer[index]);
        if (size_buffer[index] == '\n')
        {
            size_buffer[index] = '\0';
            break;
        }
        index++;
    }

    unsigned int img_size = str2uint_dec(size_buffer);
    // if(img_size == 5672){
    //     uart_display_string("size check right\n");
    // }
    // else{
    //     uart_display_string("size check wrong\n");
    // }

    uart_display_string("Start to load the kernel image\n");

    unsigned char *current = kernel_addr;
    while (img_size--)
    {
        *current = uart_get_char();
        current++;
        // uart_send('.');
    }

    uart_display_string("finishing receiving\n");
    uart_display_string("loading...\n");

    asm volatile(
        "mov x0, %0;"
        "mov x30, 0x80000;"
        "ret;"
        : /* no output registers */
        :"r" (_dtb_ptr)
        : "x0", "x30"
    );

}


