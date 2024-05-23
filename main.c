#include "uart.h"
#include "shell.h"
#include "mbox.h"

int main()
{
    // set up serial console
    uart_init();
    
    // get board info   
    show_info();
    
    // start shell
    shell_start();

    return 0;
}
