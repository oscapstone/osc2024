// #include "shell.h"
#include "stdint.h"
// #include "uart.h"

void kernel_main() {
    // initialize UART for Raspi2
    uart_init();

    shell();
}