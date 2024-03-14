#include "mini_uart.h"
#include "shell.h"
#include "string.h"


void main(){
    uart_init();
    // if(!strcmp("123", "123"))
    //   uart_writeS("same\n");
    // else uart_writeS("different\n");
    uart_sends("Hello world\n");

    shell_exec();
}