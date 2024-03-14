#include "uart.h"
#include "shell.h"

void main()
{
    // set up serial console
    uart_init();
    
    // say hello
    uart_puts("Nice to meet you!\n\r");
    int idx = 0;
    char in_char;
    // echo everything back
    while(1) {
        char buffer[1024];
        uart_puts("\r# ");
        while(1){
            in_char = uart_getc();
            uart_send(in_char);
            if(in_char == '\n'){
                buffer[idx] = '\0';
                shell(buffer);
                idx = 0;
                break;
            }
            else{
                buffer[idx] = in_char;
                idx++;
            }
        }

    }
}