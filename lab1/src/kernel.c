#include "mini_uart.h"
#include "shell.h"

void kernel_main() {
    // set up serial console
    uart_init();

    // say hello
    uart_puts("Hello World!\n");

    // start shell
    shell_start();
}