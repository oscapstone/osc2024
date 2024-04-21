#include "header/uart.h"
#include "header/shell.h"
#include "header/dtb.h"
#include "header/utils.h"
#include "header/cpio.h"
#include "header/timer.h"

extern void *_dtb_ptr;
void main(){

    	// set up serial console
    	uart_init();

	
	// say hello
	fdt_traverse(get_cpio_addr,_dtb_ptr);
        traverse_file();
	uart_send_string("# ");
	
	uart_enable_interrupt();
	except_handler_c();
	timer_irq_handler();
	irq_except_handler();

	//echo everything back
	shell();
}
