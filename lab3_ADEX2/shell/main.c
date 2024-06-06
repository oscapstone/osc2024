#include "header/uart.h"
#include "header/shell.h"
#include "header/dtb.h"
#include "header/utils.h"
#include "header/cpio.h"
#include "header/timer.h"

void test_read_command(char* buffer) {
	int index = 0;
	while(1) {
		buffer[index] = uart_get_char();
		if(buffer[index] == '\n') {
			buffer[index] = '\0';
			buffer[index+1] = '\n';
			uart_send_string("\r\n");
			break;
		}
		else 
		{
			uart_send_char(buffer[index]);
		}
		index++;
	}
}

extern void *_dtb_ptr;
void main()
{

    uart_init();
	// pre test //////////////////////////
	char buffer[256];
	while(1)    
    {
        uart_send_string("# ");
	    test_read_command(buffer);
        char * input_string = buffer;
        if(utils_string_compare(input_string,"in"))
        {
            break;
        }
        else 
        {
            // uart_send_hex((uintptr_t) _dtb_addr);
            uart_send_string("The instruct is not exist.\n");
        }
    }
	/////////////////////////////////////////

	// set up serial console
    uart_init();

	unsigned long el = 0;
	asm volatile ("mrs %0, CurrentEL":"=r"(el));
	uart_send_string("Current exception level: ");
	uart_hex(el>>2); // CurrentEL store el level at [3:2]
	uart_send_string("\r\n");

	asm volatile("mov %0, sp"::"r"(el));
	uart_send_string("Current stack pointer address: ");
	uart_hex(el);
	uart_send_string("\r\n");
	
	fdt_traverse(get_cpio_addr,_dtb_ptr);
    traverse_file();
	uart_send_string("Type in `help` to get instruction menu!\n");
	
	uart_enable_interrupt();
	

	while(1){
		asm volatile("nop");
	}
	//echo everything back
	//shell();
}


