#include "shell.h"
#include "uart.h"

int strcmp(const char* str1, const char* str2) {
    while (*str1 != '\n' || *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    if (*str1 == '\n' && *str2 =='\0') {
        return 1;
    } else {
        return 0;
    }
}

void get_command(char *command_string){
    char element;
    char *input_ptr = command_string; 
    while(1) {
        element = uart_getc();
        *input_ptr++ = element;
        if(element == '\n') {
            *input_ptr = '\0';
            uart_puts("\n");
            break;
        }
        uart_send(element);
    }
}

void load_img(){
    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    for(int i=0; i<4; i++) 
	    size_buffer[i] = uart_getb();
    uart_puts("Get kernel size.\n");

    char *kernel = (char *) 0x80000;
    while(size--) *kernel++ = uart_getb();
    uart_puts("Kernel loaded.\n");

    int r=10000; while(r--) { asm volatile("nop"); } // delay
    
    asm volatile(
        "mov x0, x24;"
        "mov x30, 0x80000;"
        "ret;"
    );

}

void shell(){
    char command_string[256];
    uart_puts("> ");
    get_command(command_string);
    if(strcmp(command_string,"help")) {
        uart_puts("help	: print this help menu\n");
        uart_puts("load	: load kernel from UART\n");
    } else if (strcmp(command_string,"load")) {
        uart_puts("Loading Kernel....\n");
        load_img();
    }     
}