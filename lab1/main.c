#include "uart.h"
#include "shell.h"
#include "mailbox.h"

void main(){
    uart_init();
    uart_puts("Hello World!\n");


    shell();
}