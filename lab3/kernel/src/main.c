
#include "io/uart.h"
#include "shell/shell.h"

void main() {

    // initialze UART
    uart_init();

    shell();

}
