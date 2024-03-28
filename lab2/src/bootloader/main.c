
#include "uart.h"

extern void* _dtb_ptr;

void main() {
    
    uart_init();

    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    for(int i=0; i < 4; i++) 
	    size_buffer[i] = uart_get_char();

    // send back for checking
    for (int i = 0; i < 4; i++)
        uart_send_char(size_buffer[i]);

    char *kernel = (char *) 0x80000;
    while(size--) *kernel++ = uart_get_char();

    uart_send_string("kernel loaded\n");
    
    asm volatile(
       "mov lr, 0x80000;"
       "ret;"
    );

}
