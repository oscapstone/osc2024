#include "header/uart.h"
#include "header/shell.h"
#include "header/reboot.h"

void main()
{
	//sasd;
    // set up serial console
    uart_init();
    // say hello
    uart_send_string("Hello World\n");
    uart_send_string("Type in `help` to get instruction menu!\n");
    // echo everything back
    shell();
}