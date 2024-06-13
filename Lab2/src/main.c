#include "../include/uart.h"
#include "../include/shell.h"
#include "../include/my_stdlib.h"

int main(){
    // set up serial console
    uart_init();

    initialize_heap();

    // start shell
    shell_start();

    return 0;
}
