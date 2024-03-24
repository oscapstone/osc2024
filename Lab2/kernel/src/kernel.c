#include "memory.h"
#include "mini_uart.h"
#include "shell.h"
#include "string.h"

void kernel_main(void)
{
    uart_init();
    uart_send_string("Welcome to Raspberry Pi 3B+\n");

    /* testing malloc */
    char* string = malloc(8);
    if (!string)
        uart_send_string("malloc failed\n");
    else {
        strcpy(string, "Hello\n");
        uart_send_string(string);
    }

    char* string2 = malloc(1000000000);
    if (!string2)
        uart_send_string("malloc failed\n");

    /* testing mem_align */
    int a;
    int* ptr = &a;
    int* ptr_align = (int*)mem_align(ptr, 16);
    uart_send_string("Original: ");
    uart_send_hex((unsigned int)ptr);
    uart_send_string("\n");
    uart_send_string("Aligned: ");
    uart_send_hex((unsigned int)ptr_align);
    uart_send_string("\n");

    shell();

    while (1)
        ;
    }
