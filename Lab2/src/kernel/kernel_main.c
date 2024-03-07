#include "kernel/gpio.h"
#include "kernel/uart.h"
#include "kernel/shell.h"
#include "kernel/cpio.h"
#include "kernel/allocator.h"
// this one need to be outside any function
char *cpio_addr = (char*)RPI_CPIO;
extern void* _dtb_addr;

void main(void)
{
    uart_init();
    uart_puts("Hello, world! 312552025\r\n");
    /*show every char typed*/
    /*while (1) {
        uart_putc(uart_getc());
    }*/
    char* string = simple_malloc(8);

    string[0] = 'S';
    string[1] = 't';
    string[2] = 'r';
    string[3] = 'i';
    string[4] = 'n';
    string[5] = 'g';
    string[6] = '!';
    string[7] = '\0';
    uart_puts(string);
    uart_putc('\n');

    my_shell();
}
