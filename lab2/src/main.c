#include "mini_uart.h"
#include "shell.h"
#include "dtb.h"
#include "utils.h"

extern void *_dtb_ptr;
void main()
{

    // set up serial console
    uart_init();
	
    // say hello
	fdt_traverse(get_cpio_addr,_dtb_ptr);
    uart_send_string("\nType in `help` to get instruction menu!\n\r");
    //echo everything back
	shell();
}
