
#include "uart.h"

extern void* _dtb_ptr;

void main() {
    
    uart_init();

    unsigned int r;

    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    for(int i=0; i < 4; i++) 
	    size_buffer[i] = uart_get_char();

    r = 500; while (r--) { asm volatile("nop"); }	
    
    // send back for checking
    for (int i = 0; i < 4; i++)
        uart_send_char(size_buffer[i]);

    char *kernel = (char *) 0x80000;
    while(size--) *kernel++ = uart_get_char();

    uart_hex64((unsigned long long)kernel);
    uart_send_string("kernel loaded.\r\n", 16);
    
    // make sure wait enough to get correct string instead of reinit uart to get wired result
	r = 500; while (r--) { asm volatile("nop"); }	

    // Jump to kernel
    asm volatile(
       "mov x1, 0x80000;"
       "br  x1;"
    );
}
