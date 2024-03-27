#include "uart.h"
#include "shell.h"
#include "dtb.h"
#include "utils.h"

extern void *_dtb_ptr;

void main()
{
	//sasd;
    // set up serial console
    uart_init();
    // say hello
    fdt_traverse(get_cpio_addr, _dtb_ptr);
    uart_send_char('\n');
    uart_send_char('\r');
    uart_display_string("Type in \"help\" to get instruction menu!\n");
    // echo everything back
    shell();
}