#define MAX_BUFFER 10
#include "include/uart.h"
#include "include/utils.h"
#include "include/mbox.h"

void load_img(){
    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    uart_puts("enter kernel size now:");
    for(int i=0; i<4; i++) 
	    size_buffer[i] = uart_getc();
    uart_puts("size-check correct\n");

    char *kernel = (char *) 0x80000;
    while(size--) *kernel++ = uart_getc();

    uart_puts("kernel-loaded\n");
}

void main(int argc, char* argv[]){
    asm volatile(
       "mov x10, x0"
    );
    uart_init();
    load_img();
    asm volatile(
       "mov x30, 0x80000;"
       "mov x0, x10;"
       "ret;"
    );
}