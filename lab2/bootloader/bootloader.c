#include "header/bootloader.h"
#include "header/uart.h"
#include "header/utils.h"
#include <stdint.h>

void read_command(char* buffer) {
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


void load_img(){
    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    for(int i=0; i<4; i++) 
	    size_buffer[i] = uart_get_char();
        // uart_send_string((char*)size_buffer);
    uart_send_string("size-check correct\n");
    char *kernel = (char *) 0x80000;

    while(size--) *kernel++ = uart_get_char();

    uart_send_string("kernel-loaded\n");
    
    char buffer[256]; 
    while(1)    
    {
        uart_send_string("# ");
	    read_command(buffer);
        char * input_string = buffer;
        if(ut_string_compare(input_string,"help"))
        {
            break;
        }
        else 
        {
            // uart_send_hex((uintptr_t) _dtb_addr);
            uart_send_string("The instruct is not exist.\n");
        }
    }
    
    
    uart_send_string("start shell kernel\n");

    asm volatile(
        "mov x30, 0x80000;"
        "ret;"
    );

}

    /* for test
    char buffer[256]; 
    while(1)    
    {
        uart_send_string("# ");
	    read_command(buffer);
        char * input_string = buffer;
        if(ut_string_compare(input_string,"help"))
        {
            break;
        }
        else uart_send_string("The instruct is not exist.\n");
    }
    */

   /*
   	ldr X5, =_dtb_addr
	str x0, [x5]
    .global _dtb_addr	//define a global variable _dtb_addr
    .section .data		//_dtb_addr is in data section
    _dtb_addr: .dc.a 0x0	//it defines _dtb_addr to be a 8-byte constant with a value of 0x0
    "ldr x1, =_dtb_addr;"
    "ldr x0, [x1];"
   */