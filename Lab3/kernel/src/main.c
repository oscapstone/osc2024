#include "printf.h"
#include "uart.h"
#include "shell.h"
#include "dtb.h"
#include "utils.h"
#include "utils_h.h"
#include "time_c.h"
#include "interrupt.h"

extern void *_dtb_ptr;

void main()
{
    uart_init();               // Set up the Uart

    init_printf(0, putc);
	int el = get_el();
    int s = get_sp();
	printf("\r\nIn Exception level: %d \r\n", el);
    printf("Stack Pinter Select is %d \r\n", s);

    // say hello
    fdt_traverse(get_cpio_addr, _dtb_ptr);
    uart_display_string("\r\n");
    
    timer_init();

    uart_display_string("\r\n");
    uart_display_string("Type in \"help\" to get instruction menu!\n");


    // echo everything back
    shell();
}