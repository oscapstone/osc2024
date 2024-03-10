
#include "shell.h"
#include "mailbox.h"
#include "reboot.h"
#include "uart.h"
#include "utils.h"

void shell() {
	
	char array_space[256];
	char* input_str = array_space;
	
	while (1) {
		char c;
		
		uart_send_string("# ");
		while (1) {
			c = uart_get_char();
			*input_str++ = c;
			uart_send_char(c);
			if (c == '\n') {
				*input_str = '\0';
				break;
			}
		}
		
		input_str = array_space;
		if (string_compare(input_str, "help")) {
			uart_send_string("help   : print this help menu\n");
			uart_send_string("hello  : print Hello World!\n");
			uart_send_string("reboot : reboot the device\n");
		} else if (string_compare(input_str, "hello")) {
			uart_send_string("Hello World!\n");
		} else if (string_compare(input_str,"info")) {
        	if (mailbox_call()) {
			get_board_revision();
			uart_send_string("Revision: ");
			uart_binary_to_hex(mailbox[5]);
			uart_send_string("\r\n");
			get_arm_mem();
			uart_send_string("Memory base address: ");
			uart_binary_to_hex(mailbox[5]);
			uart_send_string("\r\n");
			uart_send_string("Memory size: ");
			uart_binary_to_hex(mailbox[6]);
			uart_send_string("\r\n");
         } else {
           uart_send_string("Unable to query serial!\n");
         } 
     } else if (string_compare(input_str,"reboot")) {
           uart_send_string("Rebooting....\n");
           reboot(1000);
     }    
		
	}
}
