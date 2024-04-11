#include "shell.h"
#include "uart.h"
#include "timer.h"
#include "exception.h"
#include "dtb.h"
#include "tasklist.h"

extern void *_dtb_ptr;
int main()
{
    init_uart();
    mini_uart_interrupt_enable();
    //init_printf(0, putc);
    //init_memory();
    init_timer();
    fdt_traverse(get_cpio_addr,_dtb_ptr);

    uart_puts("Hello World!\n\r");

    //shell_start();
    loop();

    return 0;
}