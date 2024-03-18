#include "uart.h"

void bootloader_main()
{
    // set up serial console
    uart_init();
    int i=100;
    do{asm volatile("nop");}while(i--);
    uart_puts("Bootloader Initialized!\n\r");

    //from others
    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    for(int i=0; i<4; i++) 
	    size_buffer[i] = uart_getc();
    uart_puts("size-check correct\n");

    char *kernel = (char *) 0x80000;
    while(size--) *kernel++ = uart_getc();

    uart_puts("kernel-loaded\n");
    void (*kernel_entry)(void) = (void (*)(void))0x80000;
    kernel_entry(); 
}