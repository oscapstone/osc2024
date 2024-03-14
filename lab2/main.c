// #include "shell.h"
#include "stdint.h"
// #include "uart.h"

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
    // initialize UART for Raspi2
    uart_init();

    shell();
}