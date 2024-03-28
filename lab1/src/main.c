#include "uart.h"
#include "shell.h"

void main() {
    uart_init();
    uart_puts("hello world!\n");

    shellStart();
}
