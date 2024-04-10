#include "uart.h"
#include "shell.h"

void main()
{
    unsigned long el;
    // set up serial console
    uart_init();
    // show el
    asm volatile ("mrs %0, CurrentEL" : "=r" (el));
    el = el >> 2; 

    // print current exception level
    uart_puts("Booted! Current EL: ");
    uart_send('0' + el);
    uart_puts("\n");
    //core_timer_enable();
    int idx = 0;
    char in_char;
    // echo everything back
    while(1) {
        char buffer[1024];
        uart_send('\r');
        uart_puts("# ");
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