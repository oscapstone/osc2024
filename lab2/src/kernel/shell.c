
#include "shell.h"
#include "mailbox.h"
#include "reboot.h"
#include "uart.h"
#include "utils.h"
//#include "cpio.h"

void shell() {
	
	char array_space[256];
	char* input_str = array_space;
	
	char* cpio_addr = (char*)0x8000000;

	while (1) {
		char c;
		
		uart_send_string("# ");
		while (1) {
			c = uart_get_char();
			*input_str++ = c;
			if (c == '\n') {
				uart_send_string("\r\n");
				*input_str = '\0';
				break;
			} else {
				uart_send_char(c);
			}
		}
		
		input_str = array_space;
		if (string_compare(input_str, "help")) {
			uart_send_string("help   : print this help menu\n");
			uart_send_string("hello  : print Hello World!\n");
			uart_send_string("info   : show board info\n");
			uart_send_string("reboot : reboot the device\n");
		} else if (string_compare(input_str, "hello")) {
			uart_send_string("Hello World!\n");
		} else if (string_compare(input_str,"info")) {
			get_board_revision();
			if (mailbox_call()) {
				uart_send_string("Revision: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\r\n");
			} else {
				uart_send_string("Unable to query revision!\n");
			}
			get_arm_mem();
        	if (mailbox_call()) {
				uart_send_string("Memory base address: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\r\n");
				uart_send_string("Memory size: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[6]);
				uart_send_string("\r\n");
			} else {
				uart_send_string("Unable to query memory info!\n");
			}
			get_serial_number();
        	if (mailbox_call()) {
				uart_send_string("Serial Number: ");
    			uart_send_string("0x");
				uart_binary_to_hex(mailbox[6]);
				uart_binary_to_hex(mailbox[5]);
				uart_send_string("\r\n");
			} else {
				uart_send_string("Unable to query serial!\n");
			}
		} else if (string_compare(input_str,"reboot")) {
			uart_send_string("Rebooting....\n");
			reset(1000);
		}
		 else if (string_compare(input_str, "ls")) {
			cpio_ls(cpio_addr);
		}
		 else {
			uart_send_string("Unknown command\n");
		}
	}
}
