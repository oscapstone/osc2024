// #include "shell.h"
#include "dtb.h"
#include "stdint.h"
// #include "uart.h"

// void kernel_main() {
//     // initialize UART for Raspi3
//     uart_init();

//     register uint64_t x0 asm("x0");
//     dtb_init(x0);

//     shell();
// }

void kernel_main(char* x0) {
    dtb_init(x0);
    shell();
}